#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ncurses.h>
#include <time.h>
#include "lib.h"

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

// Responsável por criar o canal de comunicação entre o servidor e o cliente
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

    const int timeoutMillis = 1000; // 1 segundo de timeout
    struct timeval timeout = {.tv_sec = timeoutMillis / 1000, .tv_usec = (timeoutMillis % 1000) * 1000};
    setsockopt(soquete, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));

    return soquete;
}


// Atualiza o ponteiro que indica a sequencia do pacote recebido seja ACK ou NACK
void atualizaPonteiros(int *ack, int *pontsinal, int *pontsinalant, int *pontlocal)
{
    if (*ack == 1)
    {
        // Caso a sequencia do ack seja maior que o ACK anterior, mas respeitando o limite da janela
        if (*pontsinal > (*pontsinalant % TAM_JANELA))
        {
            // ponteiro sinal + quantas janelas se passaram ate esse ponteiro ( para ele nao retornar ao inicio )
            *pontsinal = *pontsinal + (((*pontsinalant - 1) / TAM_JANELA) * TAM_JANELA) + 1;
        }

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


// Recebe os dados do pacote e preenche os ponteiros com os respectivos valores
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

// Abre o terminal que simula o vi para receber o comando desejado
int terminalEntrada()
{
    FILE *arq;
    int ch;
    int charAtual = 0;
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