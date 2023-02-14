#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define TAM 8

typedef struct
{
    char caractere;
    unsigned char paridade : 1;
} byte_t;

// Preenche a paridade na vertical
void paridadeVertical(char matrizMensagem[TAM + 1], byte_t elemento[TAM + 1])
{
    int i, counter_bit, j, bit;
    int soma = 0;
    for (j = 0; j < 8; j++)
    {
        counter_bit = 0;
        for (i = 0; i < TAM; i++)
        {
            bit = (matrizMensagem[i] >> (7 - j)) & (0x01);
            if (bit)
                counter_bit++;
        }
        // nao altera o valor final
        if ((counter_bit % 2) == 0)
            soma = soma;
        else
            soma += pow(2, (7 - j));
    }
    elemento[TAM].caractere = soma;
}

// Preenche a paridade na horizontal
void paridadeHorizontal(char matrizMensagem[TAM + 1], byte_t elemento[TAM + 1])
{
    int i, counter_bit, j, bit;
    for (i = 0; i < TAM; i++)
    {
        counter_bit = 0;
        for (j = 0; j < 8; j++)
        {
            bit = (matrizMensagem[i] >> (7 - j)) & (0x01);
            if (bit)
                counter_bit++;
        }
        (elemento[i]).caractere = matrizMensagem[i];
        if ((counter_bit % 2) == 0)
            elemento[i].paridade = 0;
        else
            elemento[i].paridade = 1;
    }
}

// Busca erros na horizontal, considerando os encontrados na vertical
void erroHorizontal(byte_t elemento[TAM + 1], int coluna, int erros[TAM * 8][2], int *contaErro)
{
    int i, j, counter_bit, bit;
    for (i = 0; i < TAM; i++)
    {
        counter_bit = 0;
        for (j = 0; j < 8; j++)
        {
            bit = ((elemento[i].caractere) >> (7 - j)) & (0x01);
            if (bit)
                counter_bit++;
        }
        if ((counter_bit % 2) == 0)
        {
            if ((elemento[i].paridade) != 0)
            {
                printf(" linha %d", i);
                erros[(*contaErro)][0] = i;
                erros[(*contaErro)][1] = coluna;
                (*contaErro)++;
            }
        }
        else
        {
            if ((elemento[i].paridade) != 1)
            {
                printf(" linha %d", i);
                erros[(*contaErro)][0] = i;
                erros[(*contaErro)][1] = coluna;
                (*contaErro)++;
            }
        }
    }
}

// Imprime a matriz de hamming em binario, contendo os bytes de caracter e os bits de paridade
void imprimeMatriz(byte_t elemento[(TAM + 1)])
{
    int i = 0;
    int bit;
    int j;
    // Imprime como ficou a "tabela" do hamming code
    for (i = 0; i <= TAM; i++)
    {
        if (i == TAM)
            printf("\n");
        for (j = 0; j < 8; j++)
        {
            bit = ((elemento[i].caractere) >> (7 - j)) & (0x01);
            printf("%d", bit);
        }
        printf("  %d\n", elemento[i].paridade);
    }
    printf("\n\n");
}

int main()
{
    int i;
    int j;
    int bit;
    int counter_bit = 0;
    int contaErro = 0;
    int linha;
    int coluna;
    int soma;
    byte_t elemento[TAM + 1];
    int erros[TAM * 8][2];
    // Permitindo apenas mensagens de TAM bytes, ou seja TAM caracteres, do qual os ultimos são predestinados
    // aos bits de paridade para o hamming code.
    char matrizMensagem[TAM + 1];
    char matrizFinal[TAM];
    scanf("%s", matrizMensagem);

    // Preenche a paridade na horizontal
    paridadeHorizontal(matrizMensagem, elemento);

    // Preenche a paridade na vertical
    paridadeVertical(matrizMensagem, elemento);

    imprimeMatriz(elemento);

    // Erro gerado propositalmente a fim de teste
    elemento[0].caractere = 0x37;

    imprimeMatriz(elemento);

    // Busca erros na vertical
    for (j = 0; j < 8; j++)
    {
        counter_bit = 0;
        for (i = 0; i < TAM; i++)
        {
            bit = ((elemento[i].caractere) >> (7 - j)) & (0x01);
            if (bit)
                counter_bit++;
        }
        if ((counter_bit % 2) == 0)
        {
            if ((((elemento[TAM].caractere) >> (7 - j)) & (0x01)) != 0)
            {
                printf("Erro na coluna %d", j);
                erroHorizontal(elemento, j, erros, &contaErro);
                printf("\n");
            }
        }
        else
        {
            if ((((elemento[TAM].caractere) >> (7 - j)) & (0x01)) != 1)
            {
                printf("Erro na coluna %d", j);
                erroHorizontal(elemento, j, erros, &contaErro);
                printf("\n");
            }
        }
    }

    for (i = 0; i < contaErro; i++)
    {
        linha = erros[i][0];
        coluna = erros[i][1];

        soma = elemento[linha].caractere;
        bit = ((elemento[linha].caractere) >> (7 - coluna)) & (0x01);
        if (bit)
            soma = soma - pow(2, (7 - coluna));
        else
            soma = soma + pow(2, (7 - coluna));
        elemento[linha].caractere = soma;
    }

    for(i=0;i<TAM;i++)
    {
        matrizFinal[i]=elemento[i].caractere;
    }

    if(strncmp(matrizMensagem,matrizFinal,TAM)==0)
    {
        printf("\nMensagem: %s\n",matrizFinal);
        if(contaErro>0)
            printf("Erros corrigidos!\n");
        else
            printf("Sem erros.\n");
    }
    else
    {
        printf("Erros não podem ser corrigidos :(\n");
    }
    return 0;
}