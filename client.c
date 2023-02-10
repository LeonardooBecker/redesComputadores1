#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ncurses.h>
#include <time.h>

#define TAM 256
#define CRC 8
#define POL 0x9b

#define QNT_PACOTES 64

#define TAM_JANELA 16

#define ENCERRA_PROGRAMA -1

#define ENVIA_MENSAGEM 0

#define ENVIA_ARQUIVO 1

#define RECEBE_MENSAGEM 2

#define NADA_RECEBIDO 3

#define ERRO_FORMATACAO 4

#define RECEBE_ARQUIVO 5

typedef struct
{
    unsigned char marcadorDeInicio : 8;
    unsigned char tipo : 6;
    unsigned char sequencia : 4;
    unsigned char tamanho : 6;
    char dados[TAM];
    unsigned char crc : 8;

} pacote_t;

// Função responsável por retornar o CRC-8 com o POL predefinido em define
unsigned char retornaCRC(unsigned char *dados)
{
    int i;
    int j;

    // seq equivale a sequencia de 8 bits (1 byte)
    unsigned char seq = dados[0];
    int proximo_bit;

    // entende-se seq_crua, como a sequência sem os bits adicionais ( no caso do CRC 8, sem os 7 bits que são adicionados ao final )
    unsigned char seq_crua;

    // Sêquencia começando no primeiro byte do buffer
    seq = dados[0];

    int conta_bit = 0;

    // Percorre todos os bytes de um buffer
    for (i = 0; i < TAM; i++)
    {
        // Percorre todos os bits de um byte
        for (j = 0; j < CRC; j++)
        {
            // caso o MSB seja 1, realiza a operação xor
            if (((seq >> 7) & (0x01)) == 1)
            {
                seq = seq ^ POL;
            }

            // Caso esteja no ultimo byte do buffer
            if (conta_bit >= (TAM * CRC - CRC))
            {
                // seq_crua recebe somente os bits originais do buffer ( sem a adição do crc )
                seq_crua = (seq >> j);

                // Caso seq_crua já esteja zerada retorna então o CRC
                if ((seq_crua == 0))
                {
                    return seq << (CRC - j);
                }
            }

            // Busca o msb do proximo byte em relação ao byte atual
            if (i == (TAM - 1))
                proximo_bit = 0;
            else
                proximo_bit = (dados[i + 1] >> ((CRC - 1) - j)) & (0x01);

            seq = seq << 1;
            // Como o valor rotacionado é sempre zero, no casos em que o proximo_bit fosse 1, a adição tende ser via código
            if (proximo_bit)
                seq += 1;

            conta_bit++;
        }
    }
    return 0;
}

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

// Responsável por converter a msg de char para unsigned char
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

int cria_raw_socket(char *nome_interface_rede)
{
    struct sockaddr_ll sndr_addr;

    pacote_t pacote;
    pacote.marcadorDeInicio = 126;
    memset(pacote.dados, 0, TAM);
    pacote.tamanho = strlen(pacote.dados);

    unsigned char crc8;

    char msg[TAM];
    memset(msg, 0, TAM);

    unsigned char dados[TAM];
    memset(dados, 0, TAM);

    converteMensagem(pacote.dados, dados);

    crc8 = retornaCRC(dados);

    pacote.crc = crc8;

    // printf("\n\nCRC8: %x\n\n", pacote.crc);

    // if (testaCRC(dados, crc8))
    //     printf("Mensagem recebida corretamente");
    // else
    //     printf("Erro ao receber mensagem");

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

void atualizaPonteiros(int *ack, int *pontsinal, int *pontsinalant, int *pontlocal)
{
    if (*ack == 1)
    {
        // printf("\nACK pacote %d \n", packdevolve.sequencia);
        // Caso a sequencia do ack seja maior que o ack anterior, mas respeitando o limite da janela
        if (*pontsinal > (*pontsinalant % TAM_JANELA))
        {
            // ponteiro sinal + quantas janelas se passaram ate esse ponteiro ( para ele nao retornar ao inicio )
            *pontsinal = *pontsinal + (((*pontsinalant - 1) / TAM_JANELA) * TAM_JANELA) + 1;
        }
        //
        if (*pontsinal < (*pontsinalant % TAM_JANELA))
        {
            // ponteiro sinal + quantas janelas se passaram ate esse ponteiro e por ele ser menor, isso quer dizer que ele ja esta na proxima janela
            *pontsinal = *pontsinal + ((*pontsinalant / TAM_JANELA) * TAM_JANELA) + TAM_JANELA + 1;
        }

        if (*pontsinal == (*pontsinalant % TAM_JANELA))
        {
            *pontsinal = *pontsinalant + 1;
        }
    }

    // nack pode ser visto como um ack do valor anterior dependendo das condiçoes
    if (*ack == 0)
    {
        // printf("\nNACK pacote %d \n", packdevolve.sequencia);
        // ponteiro sinal + quantas janelas se passaram ate o ponteiro anterior ( para ele nao retornar ao inicio )
        if (*pontsinal > (*pontsinalant % TAM_JANELA))
        {
            *pontsinal = *pontsinal + (((*pontsinalant - 1) / TAM_JANELA) * TAM_JANELA);
        }
        if (*pontsinal < (*pontsinalant % TAM_JANELA))
        {
            if (*pontsinal == 0)
                *pontsinal = TAM_JANELA + ((*pontsinalant / TAM_JANELA) * TAM_JANELA);
            else
                *pontsinal = (*pontsinal - 1) + ((*pontsinalant / TAM_JANELA) * TAM_JANELA) + TAM_JANELA;
        }
        if ((*pontsinal == (*pontsinalant % TAM_JANELA)))
        {
            *pontsinal = *pontsinalant;
            *pontlocal = *pontsinal;
        }
    }
    *pontsinalant = *pontsinal;
}

void analisaDevolucao(pacote_t packdevolve, int pontsinalant, int *pontsinal, int *ack)
{
    if (packdevolve.tipo == 0x00)
    {
        *ack = 0;
        if ((packdevolve.sequencia == 0) && (pontsinalant == 0))
            *pontsinal = 0;
        else if (packdevolve.sequencia == 0)
            *pontsinal = TAM_JANELA - 1;
        else
            *pontsinal = packdevolve.sequencia;
    }
    else if (packdevolve.tipo == 0x0A)
    {
        *ack = 1;
        *pontsinal = packdevolve.sequencia;
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

int main()
{
    int soquete = cria_raw_socket("enp3s0");

    int ifindex = if_nametoindex("enp3s0");
    struct sockaddr_ll endereco = {0};
    endereco.sll_family = AF_PACKET;
    endereco.sll_protocol = htons(ETH_P_ALL);
    endereco.sll_ifindex = ifindex;

    struct sockaddr_ll sndr_addr;
    socklen_t addr_len = sizeof(struct sockaddr);

    pacote_t pacote;
    pacote_t packdevolve;
    packdevolve.sequencia = 0;
    packdevolve.tipo = 0x00;

    pacote.marcadorDeInicio = 126;
    memset(pacote.dados, 0, TAM);

    pacote.tamanho = strlen(pacote.dados);

    char linha[TAM];
    char mensagem[QNT_PACOTES][TAM];
    char mensagemCompleta[QNT_PACOTES][TAM];
    memset(mensagemCompleta, 0, TAM);
    int sequencia = 0;

    unsigned char dados[TAM];
    memset(dados, 0, TAM);

    int pontsinal = 0;
    int pontlocal = 0;
    int pontsinalant = 0;
    int ack = 0;
    int atualiza = 0;

    time_t now;
    struct tm *tm_now;
    char nomeArquivo[TAM];

    char *ptr;

    FILE *arq;

    int state;
    int stateAuxiliar;
    int packAtual = 0;

    while (1)
    {
        pacote.tipo = 0x00;
        packdevolve.tipo = 0x00;
        packAtual = 0;
        pontsinal = 0;
        pontlocal = 0;
        pontsinalant = 0;
        ack = 0;
        sequencia = 0;
        atualiza = 0;

        state = receba();
        int msg = recvfrom(soquete, &packdevolve, sizeof(packdevolve), 0, (struct sockaddr *)&sndr_addr, &addr_len);
        if (msg == -1)
        {
            if (state == NADA_RECEBIDO)
                state = NADA_RECEBIDO;
        }
        // Se ele conseguiu receber algo, é porque o outro lado enviou
        else
        {
            if (packdevolve.tipo == 0x1D)
                state = RECEBE_MENSAGEM;
        }
        while ((state == ENVIA_MENSAGEM) || (state == ENVIA_ARQUIVO))
        {
            pacote.marcadorDeInicio = 126;
            pacote.sequencia = 0;
            pacote.tipo = 0x1D;
            pacote.tamanho = 0;
            memset(pacote.dados, 61, TAM);
            pacote.crc = 0;
            int bytes_sent = sendto(soquete, &pacote, sizeof(pacote), 0, (struct sockaddr *)&endereco, sizeof(endereco));
            if (bytes_sent < 0)
                perror("sendto");
            else
                break;
        }

        if ((state == ENVIA_MENSAGEM) || (state == ENVIA_ARQUIVO))
        {
            if (state == ENVIA_MENSAGEM)
            {
                arq = fopen("msg", "r");
                if (!arq)
                    printf("Erro ao abrir o arquivo");
                while (!feof(arq))
                {
                    if (fgets(linha, TAM, arq))
                    {
                        sequencia = 0;
                        time(&now);
                        tm_now = localtime(&now);
                        memset(mensagem[sequencia], 0, TAM);
                        sprintf(mensagem[sequencia], "[%02d/%02d/%04d %02d:%02d:%02d]<%s> : %s\n",
                                tm_now->tm_mday, tm_now->tm_mon + 1, tm_now->tm_year + 1900,
                                tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec, getenv("USER"), linha);
                        // sprintf(mensagem[sequencia],"%s",linha);
                        sequencia++;
                    }
                    if (sequencia == QNT_PACOTES)
                    {
                        printf("LIMITE MAXIMO ATINGIDO");
                        break;
                    }
                }
            }

            if (state == ENVIA_ARQUIVO)
            {

                arq = fopen("msg", "r");
                fgets(linha, TAM, arq);
                ptr = strtok(linha, " ");
                ptr = strtok(NULL, " ");
                fclose(arq);

                arq = fopen(ptr, "rb");
                if (!arq)
                {
                    perror("Arquivo inexistente");
                    return 1;
                }
                int bytes_read;
                int i;
                while ((bytes_read = fread(linha, 1, TAM, arq)) > 0)
                {
                    memset(mensagem[sequencia], 0, TAM);
                    for (i = 0; i <= bytes_read; i++)
                    {
                        mensagem[sequencia][i] = linha[i];
                    }
                    sequencia++;
                }
                fclose(arq);
            }

            while (1)
            {
                if (atualiza)
                {
                    atualizaPonteiros(&ack, &pontsinal, &pontsinalant, &pontlocal);
                    atualiza = 0;
                }
                if ((pontlocal - pontsinal) < TAM_JANELA)
                {
                    // Enquanto não tiver enviado todos os pacotes, ele continua enpacotando de acordo com o pontLocal
                    if (pontlocal < sequencia)
                    {
                        // Preenchimento de pacote
                        pacote.marcadorDeInicio = 126;
                        pacote.sequencia = pontlocal;
                        if (state == ENVIA_MENSAGEM)
                            pacote.tipo = 0x01;
                        if (state == ENVIA_ARQUIVO)
                            pacote.tipo = 0x10;
                        pacote.tamanho = (unsigned char)(sizeof(mensagem[pontlocal]));
                        memset(pacote.dados, 0, TAM);
                        for (int i = 0; i < sizeof(mensagem[pontlocal]); i++)
                        {
                            pacote.dados[i] = mensagem[pontlocal][i];
                            // if(mensagem[pontlocal][i]==0x00)
                            // {
                            //     break;
                            // }
                        }
                        memset(dados, 0, TAM);
                        converteMensagem(pacote.dados, dados);
                        pacote.crc = retornaCRC(dados);
                        int bytes_sent = sendto(soquete, &pacote, sizeof(pacote), 0, (struct sockaddr *)&endereco, sizeof(endereco));
                        if (bytes_sent < 0)
                            perror("sendto");
                        else
                            pontlocal++;
                    }
                    // Envia pacote que sinaliza o fim da sequência e sai do while
                    if ((pontlocal == sequencia) && (ack == 1))
                    {

                        // Escreve somente o tipo ~ fim ( 0x0F ) no pacote
                        pacote.marcadorDeInicio = 126;
                        pacote.sequencia = pontlocal;
                        pacote.tipo = 0x0F;
                        pacote.tamanho = 0;
                        memset(pacote.dados, 61, TAM);
                        memset(dados, 0, TAM);
                        converteMensagem(pacote.dados, dados);
                        pacote.crc = retornaCRC(dados);

                        int bytes_sent = sendto(soquete, &pacote, sizeof(pacote), 0, (struct sockaddr *)&endereco, sizeof(endereco));
                        if (bytes_sent < 0)
                            perror("sendto");
                        else
                            break;
                    }
                }
                // Recebe mensagem de ack ou nack
                int msg = recvfrom(soquete, &packdevolve, sizeof(packdevolve), 0, (struct sockaddr *)&sndr_addr, &addr_len);
                if (msg == -1)
                    perror("Erro ao receber a mensagem");
                else
                {
                    // printf("\n");
                    // printf("Marcador de inicio: %d\n", packdevolve.marcadorDeInicio);
                    // printf("Tipo:%x\n", packdevolve.tipo);
                    // printf("Sequencia: %x\n", packdevolve.sequencia);
                    // printf("Tamanhooo: %d\n", packdevolve.tamanho);
                    // printf("Dados:%s\n", packdevolve.dados);
                    // printf("CRC:%x\n", packdevolve.crc);
                    // printf("\n");
                    atualiza = 1;
                }

                analisaDevolucao(packdevolve, pontsinalant, &pontsinal, &ack);
            }
        }
        // printf("OIIIIIIIIIII\n\n\n\n");
        if (state == RECEBE_MENSAGEM)
        {
            packAtual = 0;
            pacote.tipo = 0x00;
            while (pacote.tipo != 0x0F)
            {
                memset(packdevolve.dados, 0, TAM);
                packdevolve.crc = 0x00;
                msg = recvfrom(soquete, &pacote, sizeof(pacote), 0, (struct sockaddr *)&sndr_addr, &addr_len);
                if (msg == -1)
                    perror("Erro ao receber a mens");
                else
                {
                    // printf("\n");
                    // printf("Marcador de inicio: %d\n", pacote.marcadorDeInicio);
                    // printf("Tipo:%x\n", pacote.tipo);
                    // printf("Sequencia: %x\n", pacote.sequencia);
                    // printf("Tamanho: %d\n", pacote.tamanho);
                    // printf("Dados:%s\n", pacote.dados);
                    // printf("CRC:%x\n", pacote.crc);
                    // printf("\n");
                    if ((pacote.tipo == 0x01) || (pacote.tipo == 0x10))
                    {
                        if (pacote.tipo == 0x01)
                            stateAuxiliar = RECEBE_MENSAGEM;
                        if (pacote.tipo == 0x10)
                            stateAuxiliar = RECEBE_ARQUIVO;
                        memset(mensagemCompleta[packAtual], 0, TAM);
                        for (int i = 0; i < TAM; i++)
                            mensagemCompleta[packAtual][i] = pacote.dados[i];
                        // strncpy(mensagemCompleta[packAtual],pacote.dados,TAM);
                        packAtual++;
                    }
                    memset(dados, 0, TAM);
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
                        packdevolve.tipo = 0x00;
                        packdevolve.sequencia = pacote.sequencia;
                        // printf("Erro ao receber mensagem crc");
                    }
                }
                int bytes_sent = sendto(soquete, &packdevolve, sizeof(packdevolve), 0, (struct sockaddr *)&endereco, sizeof(endereco));
                if (bytes_sent < 0)
                {
                    perror("sendto");
                }
            }
            FILE *arq2;
            if (stateAuxiliar == RECEBE_ARQUIVO)
            {
                printf("midia enviada\n\n");
                arq2 = fopen("img.jpg", "wb");
                for (int i = 0; i < packAtual; i++)
                {
                    fwrite(mensagemCompleta[i], 1, sizeof(mensagemCompleta[i]), arq2);
                }
                fclose(arq2);
            }
            if (stateAuxiliar == RECEBE_MENSAGEM)
            {
                for (int i = 0; i < packAtual; i++)
                {
                    printf("%s", mensagemCompleta[i]);
                }
            }
        }

        state = NADA_RECEBIDO;
    }

    printf("\n state:%d <> <> All packets send\n", state);
}