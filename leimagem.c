#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define QNT_PACOTES 256
#define TAM 256
#define CRC 8
#define POL 0x9b

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

int main()
{
    char linha[256];
    char mensagem[64][256];
    int sequencia = 0;
    char *ptr;
    FILE *aux;
    aux=fopen("msg","r");
    fgets(linha,TAM,aux);
    ptr=strtok(linha," ");
    ptr=strtok(NULL," ");
    printf("%s",ptr);

    FILE *arq;
    arq = fopen("antena.jpg", "rb");
    FILE *arq2;
    arq2 = fopen("devolve.jpg", "wb");
    int i = 0;
    int bytes_read;
    int tamanho = 0;
    unsigned char dados[TAM];
    unsigned char crc8;
    while ((bytes_read = fread(linha, 1, sizeof(linha), arq)) > 0)
    {
        // fwrite(linha, 1, sizeof(linha), arq2);

        for (i = 0; i < bytes_read; i++)
            mensagem[sequencia][i] = linha[i];
        memset(dados, 0, TAM);
        converteMensagem(mensagem[sequencia], dados);
        crc8 = retornaCRC(dados);
        if (testaCRC(dados, crc8))
            printf("Mensagem recebida corretamente");
        else
            printf("Erro ao receber mensagem");

        // strcpy(mensagem[sequencia], linha);
        // tamanho = sizeof(mensagem[sequencia]);
        // // printf("%s // %ld\n\n\n", linha, strlen(linha));
        // // printf("\n");
        // memset((mensagem[sequencia] + sizeof(mensagem[sequencia])), 61, TAM);
        // // printf("%s\n\n\n",mensagem[sequencia]);

        // memset((mensagem[sequencia] + tamanho), 0, TAM);
        fwrite(mensagem[sequencia], 1, sizeof(mensagem[sequencia]), arq2);

        // printf("%s // %ld\n\n\n", mensagem[sequencia], strlen(mensagem[sequencia]));
        sequencia++;
    }

    printf("\n\n%d\n\n", i);
}