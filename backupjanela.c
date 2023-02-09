#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAM 256
#define CRC 8
#define POL 0x9b

// tamanho de cada quadro == 256 bytes
// tamanho maximo de um arquivo = 1 MB
// quantidade de pacotes maximos a ser enviado em uma mensagem = 2^20/2^8 = 2^12

#define QNT_PACOTES 4096

int main()
{

    int i;
    char linha[TAM];
    char mensagem[QNT_PACOTES][TAM];
    int sequencia = 0;

    FILE *arq;
    arq = fopen("estouro", "r");
    printf("%ld", sizeof(arq));
    while (!feof(arq))
    {
        if (fgets(linha, TAM, arq))
        {
            printf("\n%ld\n", strlen(linha));
            strcpy(mensagem[sequencia], linha);
            sequencia++;
        }
    }
    for (i = 0; i < sequencia; i++)
    {
        printf("%s\n", mensagem[i]);
    }

    int qntquadro = 36;
    int j;

    int pontsinal = 0;
    int pontlocal = 0;
    int pontsinalant = 0;
    int distanciaMaxima = 8;
    int ack;
    int valor;
    sequencia = 36;
    while (pontsinal < sequencia)
    {
        if ((pontlocal - pontsinal > (distanciaMaxima - 1)) || (pontlocal == (sequencia)))
        {
            pontsinalant = pontsinal;
            scanf("%d", &pontsinal);
            while ((pontsinal < 0) || (pontsinal > 7))
                scanf("%d", &pontsinal);
            scanf("%d", &ack);

            // caso ack
            if (ack)
            {
                if (pontsinal > (pontsinalant % distanciaMaxima))
                {
                    pontsinal = pontsinalant + (pontsinal - (pontsinalant % distanciaMaxima)) + 1;
                }
                else if (pontsinal < (pontsinalant % distanciaMaxima))
                {
                    pontsinal = pontsinalant + distanciaMaxima - (pontsinalant % distanciaMaxima) + pontsinal + 1;
                }
                else
                {
                    pontsinal = pontsinalant + 1;
                }
            }

            //caso nack
            if (ack == 0)
            {

                if (pontsinal == (pontsinalant % distanciaMaxima))
                {
                    pontlocal = pontsinalant;
                    pontsinal = pontlocal;
                }
                else if (pontsinal > (pontsinalant % distanciaMaxima))
                {
                    pontsinal = pontsinalant + (pontsinal - (pontsinalant % distanciaMaxima));
                }
                else if (pontsinal < (pontsinalant % distanciaMaxima))
                {
                    pontsinal = pontsinalant + distanciaMaxima - (pontsinalant % distanciaMaxima) + pontsinal;
                }
            }
        }

        printf("\n");
        // printf("pontlocal: %d // pontsinal: %d // pontsinalant: %d // valor%d\n", pontlocal, pontsinal, pontsinalant, (pontlocal % distanciaMaxima));
        if (pontlocal < sequencia)
        {
            printf("enviando pacote: %d\n", pontlocal);
            pontlocal++;
        }

    }
    printf("\nAll packets send\n");
}