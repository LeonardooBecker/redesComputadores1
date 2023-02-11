#ifndef _libAux_
#define _libAux_

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
unsigned char retornaCRC(unsigned char *dados);

// Função responsável por receber a mensagem, o CRC, e realizar a validação
int testaCRC(unsigned char *dados, unsigned char crc);

// Responsável por converter a msg de char para unsigned char
void converteMensagem(char *msg, unsigned char *dados);

// Responsável por criar o canal de comunicação entre o servidor e o cliente
int cria_raw_socket(char *nome_interface_rede);

// Atualiza o ponteiro que indica a sequencia do pacote recebido seja ACK ou NACK
void atualizaPonteiros(int *ack, int *pontsinal, int *pontsinalant, int *pontlocal);

// Recebe os dados do pacote e preenche os ponteiros com os respectivos valores
void analisaDevolucao(pacote_t packdevolve, int pontsinalant, int *pontsinal, int *ack);

// Abre o terminal que simula o vi para receber o comando desejado
int terminalEntrada();


#endif