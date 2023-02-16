#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TAM 256
#define CRC 8
#define POL 0x9b

// tamanho de cada quadro == 256 bytes
// tamanho maximo de um arquivo = 1 MB
// quantidade de pacotes maximos a ser enviado em uma mensagem = 2^20/2^8 = 2^12

#define QNT_PACOTES 4096

#define TAM_JANELA 16

typedef struct
{
    unsigned char marcadorDeInicio : 8;
    unsigned char tipo : 6;
    unsigned char sequencia : 4;
    unsigned char tamanho : 6;
    char dados[TAM];
    unsigned char crc;

} pacote_t;

int main()
{

    int i;
    char linha[TAM];
    char mensagem[QNT_PACOTES][TAM];
    int sequencia = 0;

    srand(time(NULL));
    int tipo = 0;
    int seq = 0;

    FILE *arq;
    arq = fopen("estouro", "r");
    while (!feof(arq))
    {
        if (fgets(linha, TAM, arq))
        {
            strcpy(mensagem[sequencia], linha);
            sequencia++;
        }
    }

    int pontsinal = 0;
    int pontlocal = 0;
    int pontsinalant = 0;
    int ack;
    int att = 0;

    while (1)
    {
        if (att)
        {
            if (ack == 1)
            {
                printf("\nACK pacote %d \n", seq);
                // Caso a sequencia do ack seja maior que o ack anterior, mas respeitando o limite da janela
                if (pontsinal > (pontsinalant % TAM_JANELA))
                {
                    // ponteiro sinal + quantas janelas se passaram ate esse ponteiro ( para ele nao retornar ao inicio )
                    pontsinal = pontsinal + (((pontsinalant - 1) / TAM_JANELA) * TAM_JANELA) + 1;
                }
                //
                if (pontsinal < (pontsinalant % TAM_JANELA))
                {
                    // ponteiro sinal + quantas janelas se passaram ate esse ponteiro e por ele ser menor, isso quer dizer que ele ja esta na proxima janela
                    pontsinal = pontsinal + ((pontsinalant / TAM_JANELA) * TAM_JANELA) + TAM_JANELA + 1;
                }

                if (pontsinal == (pontsinalant % TAM_JANELA))
                {
                    pontsinal = pontsinalant + 1;
                }
            }

            // nack pode ser visto como um ack do valor anterior dependendo das condiçoes
            if (ack == 0)
            {
                printf("\nNACK pacote %d \n", seq);
                // ponteiro sinal + quantas janelas se passaram ate o ponteiro anterior ( para ele nao retornar ao inicio )
                if (pontsinal > (pontsinalant % TAM_JANELA))
                {
                    pontsinal = pontsinal + (((pontsinalant - 1) / TAM_JANELA) * TAM_JANELA);
                }
                if (pontsinal < (pontsinalant % TAM_JANELA))
                {
                    if (pontsinal == 0)
                        pontsinal = TAM_JANELA + ((pontsinalant / TAM_JANELA) * TAM_JANELA);
                    else
                        pontsinal = (pontsinal - 1) + ((pontsinalant / TAM_JANELA) * TAM_JANELA) + TAM_JANELA;
                }
                if ((pontsinal == (pontsinalant % TAM_JANELA)))
                {
                    pontsinal = pontsinalant;
                    pontlocal = pontsinal;
                }
            }
            att = 0;
        }

        pontsinalant = pontsinal;

        if ((pontlocal - pontsinal) < TAM_JANELA)
        {

            if ((pontsinalant == sequencia) && (ack == 1))
            {
                printf("\nSequencia finalizada\n\n");
                break;
            }

            // MENSAGEM FOI ENVIADA AQUI

            // Enquanto não tiver enviado todos os pacotes, ele continua enpacotando de acordo com o pontLocal
            if (pontlocal < sequencia)
            {
                pontlocal++;
            }

            // RECEBE O ACK
        }

        printf("pacote enviado: ");
        for (i = pontsinal; i < pontlocal; i++)
        {
            printf("%d ", i % TAM_JANELA);
        }
        printf("\n");

        // 25% de chance de gerar um ack ou nack
        if ((rand() % 4) == 0)
        {
            seq = (pontsinal + rand() % (pontlocal - pontsinal)) % TAM_JANELA;

            // 70% de chance de ser um ACK
            if ((rand() % 10) < 7)
                tipo = 1;
            else
                tipo = 0;
            att = 1;
            // 10% de chance de ter que renviar todos os pacotes
            if ((rand() % 10) > 1)
            {
                tipo = 0;
                seq = (pontsinal % TAM_JANELA);
            }
        }

        if (att)
        {
            if (tipo == 0)
            {
                ack = 0;
                if ((seq == 0) && (pontsinalant == 0))
                    pontsinal = 0;
                else if (seq == 0)
                    pontsinal = TAM_JANELA - 1;
                else
                    pontsinal = seq;
            }
            else if (tipo == 1)
            {
                ack = 1;
                pontsinal = seq;
            }
        }
    }
    printf("\nAll packets send\n");
}