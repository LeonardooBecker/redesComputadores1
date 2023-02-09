#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAM 1
#define CRC 8
#define POL 0x9b

// tamanho de cada quadro == 256 bytes
// tamanho maximo de um arquivo = 1 MB
// quantidade de pacotes maximos a ser enviado em uma mensagem = 2^20/2^8 = 2^12

#define QNT_PACOTES 4096

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

            printf("%x\n", seq);

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

int main()
{
    int i = 0;

    char linha[TAM];
    char mensagem[QNT_PACOTES][TAM];
    int sequencia = 0;

    unsigned char crc8;

    char msg[1];
    memset(msg, 0, TAM);

    unsigned char dados[TAM];
    memset(dados, 0, TAM);

    // printf("Digite uma mensagem:\n");
    // fgets(msg, TAM, stdin);
    // printf("%s",msg);
    // converteMensagem(msg,dados);
    // printf("\nstrlen:%ld\n",strlen(msg));
    // crc8 = retornaCRC(dados);
    // printf("\ncrc8:%x\n",crc8);

    // FILE *arq;
    // arq = fopen("msg", "r");
    // printf("%ld", sizeof(arq));
    // while (!feof(arq))
    // {
    //     if (fgets(linha, TAM, arq))
    //     {
    //         // printf("\n%ld\n", strlen(linha));
    //         strcpy(mensagem[sequencia], linha);
    //         sequencia++;
    //     }
    // }
    msg[0] = 0x61;
    converteMensagem(msg, dados);
    // converteMensagem(mensagem[0],dados);
    crc8 = retornaCRC(dados);
    printf("%x", msg[0]);
    printf("\ncrc8:%x\n", crc8);

    if (testaCRC(dados, crc8))
        printf("Mensagem recebida corretamente");
    else
        printf("Erro ao receber mensagem");
}
