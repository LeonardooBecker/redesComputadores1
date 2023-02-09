#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define TAM 256
#define CRC 8
#define POL 0x9b
#define QUADRO 16

typedef struct
{
    unsigned char marcadorDeInicio : 8;
    unsigned char tipo : 6;
    unsigned char sequencia : 4;
    unsigned char tamanho : 6;
    char dados[TAM];
    unsigned char crc;

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

int cria_raw_socket(char *nome_interface_rede)
{

    pacote_t pacote;

    struct sockaddr_ll sndr_addr;
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

    unsigned char nbuffer[1024];
    int msg = 0;
    socklen_t addr_len = sizeof(struct sockaddr);
    msg = recvfrom(soquete, &pacote, 1029, 0, (struct sockaddr *)&sndr_addr, &addr_len);
    if (msg == -1)
        perror("Erro ao receber a mensagem");
    else
    {
        printf("Marcador de inicio: %d\n", pacote.marcadorDeInicio);
        printf("Tamanho: %d\n", pacote.tamanho);
        printf("Dados:%s\n", pacote.dados);
        printf("CRC:%x\n", pacote.crc);
    }

    unsigned char dados[TAM];
    converteMensagem(pacote.dados,dados);
    if (testaCRC(dados, pacote.crc))
        printf("Mensagem recebida corretamente");
    else
        printf("Erro ao receber mensagem");

    unsigned char ack[QUADRO] = "1111";
    int sent_len = sendto(soquete, ack, sizeof(ack), 0,
                          (struct sockaddr *)&endereco, sizeof(endereco));
    if (sent_len < 0)
    {
        perror("sendto");
    }
    printf("%s", ack);

    return soquete;
}

int main()
{
    int soquete = cria_raw_socket("lo");
}