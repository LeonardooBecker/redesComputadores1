#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ncurses.h>

#define TAM 256
#define CRC 8
#define POL 0x9b
#define QUADRO 16
#define QNT_PACOTES 64
#define TAM_JANELA 16

#define ENCERRA_PROGRAMA -1
#define ENVIA_MENSAGEM 0
#define ENVIA_ARQUIVO 1
#define RECEBE_MENSAGEM 2
#define NADA_RECEBIDO 3
#define ERRO_FORMATACAO 4

typedef struct
{
    unsigned char marcadorDeInicio : 8;
    unsigned char tipo : 6;
    unsigned char sequencia : 4;
    unsigned char tamanho : 6;
    char dados[TAM];
    unsigned char crc:8;

} pacote_t;

// Função responsável por receber a mensagem, o CRC, e realizar a validação
int testaCRC(unsigned char *dados, unsigned char crc)
{
    int i;

    // Array com um byte a mais, do qual sera inserido o CRC
    unsigned char dadosComp[TAM + 1];

    for (i = 0; i < TAM; i++)
        dadosComp[i] = dados[i];

    // Inserção do CRC no final do buffer
    dadosComp[TAM] = crc;

    int j;
    int proximo_bit;

    // seq equivale a sequencia de 8 bits (1 byte)
    unsigned char seq = dadosComp[0];

    int conta_bit = 0;

    // Percorre todos os bytes de um buffer
    for (i = 0; i < TAM; i++)
    {
        // Percorre todos os bits de um byte
        for (j = 0; j < CRC; j++)
        {
            // Caso o MSB seja 1, realiza a operação xor
            if (((seq >> 7) & (0x01)) == 1)
            {
                seq = seq ^ POL;
            }

            // Busca o msb do proximo byte em relação ao byte atual
            proximo_bit = (dadosComp[i + 1] >> ((CRC - 1) - j)) & (0x01);

            // Como o valor rotacionado é sempre zero, no casos em que o proximo_bit fosse 1, a adição tende ser via código
            seq = seq << 1;
            if (proximo_bit)
                seq += 1;

            conta_bit++;
        }
    }
    if (seq == 0)
        return 1;
    else
        return 0;
}

void converteMensagem(char *msg, unsigned char *dados)
{
    int i;
    for (i = 0; i < TAM; i++)
    {
        // printf("%x ", msg[i]);
        if (msg[i] != 0xa)
            dados[i] = (unsigned char)msg[i];
    }
}

int receba()
{
    FILE *arq;
    int ch;
    int charAtual = 0;
    char linha[TAM];
    int sequencia = 0;
    char read[TAM * QNT_PACOTES];

    int inicio = 0;

    memset(read, 0, TAM);
    initscr();            // Inicializa o ncurses
    cbreak();             // Desativa o buffer de entrada
    keypad(stdscr, TRUE); // Ativa o suporte a teclas especiais
    timeout(5);

    // Recebe possívelmente uma tecla
    ch = getch();
    clear();
    inicio = ch;
    if ((ch == 105) || (ch == 58))
    {
        if (ch == 105)
            printw("Insira sua mensagem: ( Enter para enviar )\n");
        if (ch == 58)
            printw(":");

        // Enquanto o caractere recebido for diferente de enter
        while (1)
        {
            ch = getch();
            // timeout nao altera em nada
            if (ch != ERR)
            {
                // Se a tecla for para apagar caractere
                if ((ch == 263) && (charAtual > 0))
                {
                    charAtual--;
                    read[charAtual] = 0x00;
                }
                else
                {
                    read[charAtual] = ch;
                    charAtual++;
                }
            }

            // se o caractere digitado for esc
            if (ch == 27)
            {
                memset(read, 0, TAM);
                endwin();
                return NADA_RECEBIDO;
            }

            // precisa do enter no final da string a fim da comparação posterior
            if (ch == 10)
                break;
        }
    }
    else
    {
        endwin();
        return NADA_RECEBIDO;
    }

    endwin(); // Finaliza o ncurses

    // char :
    if (inicio == 58)
    {
        // q - encerra o programa
        if ((read[0] == 113) && (read[1] == 10))
        {
            printf("Encerrando programa");
            return ENCERRA_PROGRAMA;
        }

        // send - envia arquivo
        if ((read[0] == 115) && (read[1] == 101) && (read[2] == 110) && (read[3] == 100) && (read[4] == 32))
        {
            arq = fopen("msg", "w");
            if (!arq)
                printf("Erro ao abrir o arquivo");
            read[(strlen(read) - 1)] = 0x00;
            fprintf(arq, "%s", read);
            fclose(arq);
            return ENVIA_ARQUIVO;
        }
    }
    else if (inicio == 105)
    {
        arq = fopen("msg", "w");
        if (!arq)
            printf("Erro ao abrir o arquivo");
        fprintf(arq, "%s", read);
        fclose(arq);
        return ENVIA_MENSAGEM;
    }
    else
        return ERRO_FORMATACAO;
    return NADA_RECEBIDO;
}

int cria_raw_socket(char *nome_interface_rede)
{

    // Cria arquivo para o socket sem qualquer protocolo
    int soquete = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (soquete == -1)
    {
        fprintf(stderr, "Erro ao criar socket: Verifique se você é root!\n");
        exit(-1);
    }
    int ifindex = if_nametoindex(nome_interface_rede);
    struct sockaddr_ll endereco = {0};
    endereco.sll_family = AF_PACKET;
    endereco.sll_protocol = htons(ETH_P_ALL);
    endereco.sll_ifindex = ifindex;

    // Inicializa socket
    if (bind(soquete, (struct sockaddr *)&endereco, sizeof(endereco)) == -1)
    {
        fprintf(stderr, "Erro ao fazer bind no socket\n");
        exit(-1);
    }
    
    struct packet_mreq mr = {0};
    mr.mr_ifindex = ifindex;
    mr.mr_type = PACKET_MR_PROMISC;
    // Não joga fora o que identifica como lixo: Modo promíscuo
    if (setsockopt(soquete, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) == -1)
    {
        fprintf(stderr, "Erro ao fazer setsockopt: "
                        "Verifique se a interface de rede foi especificada corretamente.\n");
        perror("erro");
    }

    const int timeoutMillis = 1000; // 300 milisegundos de timeout por exemplo
    struct timeval timeout = {.tv_sec = timeoutMillis / 1000, .tv_usec = (timeoutMillis % 1000) * 1000};
    setsockopt(soquete, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));

    return soquete;
}

int main()
{
    int soquete = cria_raw_socket("enp0s7");

    int ifindex = if_nametoindex("enp0s7");
    struct sockaddr_ll endereco = {0};
    endereco.sll_family = AF_PACKET;
    endereco.sll_protocol = htons(ETH_P_ALL);
    endereco.sll_ifindex = ifindex;
    struct sockaddr_ll sndr_addr;
    socklen_t addr_len = sizeof(struct sockaddr);


    pacote_t pacote;

    pacote_t packdevolve;

    unsigned char nbuffer[1024];
    int msg = 0;

    pacote.tamanho = 0;
    pacote.sequencia = -1;

    packdevolve.marcadorDeInicio=126;
    packdevolve.tamanho=0;
    packdevolve.tipo=0x00;
    memset(packdevolve.dados,0,TAM);
    packdevolve.crc=0;

    unsigned char dados[TAM];

    char mensagemCompleta[QNT_PACOTES][TAM];
    memset(mensagemCompleta,0,TAM);
    int packAtual=0;

    

    // packdevolve.tipo=0x1D;
    // int bytes_sent = sendto(soquete, &packdevolve, sizeof(packdevolve), 0, (struct sockaddr *)&endereco, sizeof(endereco));
    // if (bytes_sent < 0)
    // {
    //     perror("sendto");
    // }

    int state;
    // while (1)
    // {
    //     state = receba();

    //     if ((state == ENVIA_MENSAGEM) || (state == ENVIA_ARQUIVO))
    //     {
    //         pacote.marcadorDeInicio = 126;
    //         pacote.sequencia = 0;
    //         pacote.tipo = 0x1D;
    //         pac ote.tamanho = 0;
    //         memset(pacote.dados, 0, TAM);
    //         pacote.crc = 0;
    //         int bytes_sent = sendto(soquete, &pacote, sizeof(pacote), 0, (struct sockaddr *)&endereco, sizeof(endereco));
    //         if (bytes_sent < 0)
    //             perror("sendto");
    //         break;
    //     }

    //     if(state==NADA_RECEBIDO)
    //     { 
    //         int msg = recvfrom(soquete, &packdevolve, sizeof(packdevolve), 0, (struct sockaddr *)&sndr_addr, &addr_len);
    //         if (msg == -1)
    //             perror("Erro ao receber a mensagem aqui");
    //         else
    //         {
    //             state = RECEBE_MENSAGEM;
    //             break;
    //         }      
    //     }  
    // }

    while(1)
    {
        pacote.tipo=0x00;
        packdevolve.tipo=0x00;
        packAtual=0;
        state = receba();
        msg = recvfrom(soquete, &pacote, sizeof(pacote), 0, (struct sockaddr *)&sndr_addr, &addr_len);
        if (msg == -1)
        {
            if(state==NADA_RECEBIDO)
                state=NADA_RECEBIDO;
        }
        // Se conseguiu receber algo, foi porque o outro lado enviou
        else
        {
            //preciso mudar state
            state=RECEBE_MENSAGEM;
            printf("\n");
        }
        if(state==RECEBE_MENSAGEM)
        {
            while (pacote.tipo!=0x0F)
            {
                msg = recvfrom(soquete, &pacote, sizeof(pacote), 0, (struct sockaddr *)&sndr_addr, &addr_len);
                if (msg == -1)
                    perror("Erro ao receber a mens");
                else
                {
                        // printf("\n");
                        // printf("Marcador de inicio: %d\n", pacote.marcadorDeInicio);
                        // printf("Tipo:%x\n",pacote.tipo);
                        // printf("Sequencia: %x\n", pacote.sequencia);
                        // printf("Tamanho: %d\n", pacote.tamanho);
                        // printf("Dados:%s\n", pacote.dados);
                        // printf("CRC:%x\n", pacote.crc);
                        // printf("\n");
                        if(pacote.tipo==0x01)
                        {
                            memset(mensagemCompleta[packAtual],0,TAM);
                            for(int i=0;i<TAM;i++)
                                 mensagemCompleta[packAtual][i]=pacote.dados[i];
                            // strncpy(mensagemCompleta[packAtual],pacote.dados,TAM);
                            packAtual++;
                        }
                    memset(dados,0,TAM);
                    converteMensagem(pacote.dados, dados);
                    if (testaCRC(dados, pacote.crc))
                    {
                        // Tipo de devolução ack                
                        packdevolve.tipo = 0x0A;
                        packdevolve.sequencia = pacote.sequencia;
                        // printf("Mensagem recebida corretamente");
                    }
                    else
                    {
                        packdevolve.tipo=0x00;
                        packdevolve.sequencia = pacote.sequencia;
                        // printf("Erro ao receber mensagem crc");
                    }

                    int bytes_sent = sendto(soquete, &packdevolve, sizeof(packdevolve), 0, (struct sockaddr *)&endereco, sizeof(endereco));
                    if (bytes_sent < 0)
                    {
                        perror("sendto");
                    }
                 }
             }
        
            FILE *arq2;
            arq2=fopen("img.jpg","wb");
            for(int i=0;i<packAtual;i++)
            {
                printf("%s",mensagemCompleta[i]);
                fwrite(mensagemCompleta[i],1,sizeof(mensagemCompleta[i]),arq2);
            }
            fclose(arq2);
        }
        state=NADA_RECEBIDO;
    }
}