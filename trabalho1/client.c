#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ncurses.h>
#include <time.h>
#include <unistd.h>
#include "lib.h"

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

    char diretorio[TAM];
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

    char *ptr;

    FILE *arq;

    int state;
    int stateAuxiliar;
    int packAtual = 0;
    int timeout = 0;

    FILE *log;
    log = fopen("log.txt", "w");
    if (!log)
    {
        perror("Erro ao abrir arquivo");
    }
    fprintf(log, "\t\tIniciando batebapo\n\n");

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

        state = terminalEntrada();
        if (state == ENCERRA_PROGRAMA)
        {
            printf("\nSucesso\n");
            return 200;
        }
        int msg = recvfrom(soquete, &packdevolve, sizeof(packdevolve), 0, (struct sockaddr *)&sndr_addr, &addr_len);
        if (msg == -1)
        {
            if (state == NADA_RECEBIDO)
                state = NADA_RECEBIDO;
            else
            {
                timeout++;
                if (timeout == 15)
                {
                    perror("Timeout");
                    return 201;
                }
            }
        }
        // Se ele conseguiu receber algo, ?? porque o outro lado enviou
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
                {
                    perror("Arquivo inexistente");
                    return 205;
                }
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

                        fprintf(log, "Mensagem enviada: [%02d/%02d/%04d %02d:%02d:%02d]<%s> : %s\n",
                                tm_now->tm_mday, tm_now->tm_mon + 1, tm_now->tm_year + 1900,
                                tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec, getenv("USER"), linha);

                        sequencia++;
                    }
                    if (sequencia == QNT_PACOTES)
                    {
                        perror("Falha ao escrever arquivo");
                        return 207;
                    }
                }
                fclose(arq);
            }

            if (state == ENVIA_ARQUIVO)
            {
                getcwd(diretorio, sizeof(diretorio));
                arq = fopen("msg", "r");
                if (!arq)
                {
                    perror("Arquivo inexistente");
                    return 205;
                }

                fgets(linha, TAM, arq);
                ptr = strtok(linha, " ");
                ptr = strtok(NULL, " ");
                fclose(arq);

                arq = fopen(ptr, "rb");
                if (!arq)
                {
                    perror("Arquivo inexistente");
                    return 205;
                }

                fprintf(log, "Midia enviada: [%02d/%02d/%04d %02d:%02d:%02d]<%s> : <%s/%s>\n\n",
                        tm_now->tm_mday, tm_now->tm_mon + 1, tm_now->tm_year + 1900,
                        tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec, getenv("USER"), diretorio, ptr);

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

            timeout = 0;
            while (1)
            {
                if (atualiza)
                {
                    atualizaPonteiros(&ack, &pontsinal, &pontsinalant, &pontlocal);
                    atualiza = 0;
                }
                if ((pontlocal - pontsinal) < TAM_JANELA)
                {
                    // Enquanto n??o tiver enviado todos os pacotes, ele continua enpacotando de acordo com o pontLocal
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
                    // Envia pacote que sinaliza o fim da sequ??ncia e sai do while
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
                {
                    timeout++;
                    if (timeout == 10)
                    {
                        perror("Timeout");
                        return 201;
                    }
                }
                else
                {
                    atualiza = 1;
                }

                analisaDevolucao(packdevolve, pontsinalant, &pontsinal, &ack);
            }
        }
        if (state == RECEBE_MENSAGEM)
        {
            packAtual = 0;
            pacote.tipo = 0x00;
            timeout = 0;
            while (pacote.tipo != 0x0F)
            {
                memset(packdevolve.dados, 0, TAM);
                packdevolve.crc = 0x00;
                msg = recvfrom(soquete, &pacote, sizeof(pacote), 0, (struct sockaddr *)&sndr_addr, &addr_len);
                if (msg == -1)
                {
                    timeout++;
                    if (timeout == 10)
                    {
                        perror("Timeout");
                        return 201;
                    }
                }
                else
                {
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
                        // Tipo de devolu????o ack
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
            FILE *arqImagem;
            if (stateAuxiliar == RECEBE_ARQUIVO)
            {
                time(&now);
                tm_now = localtime(&now);
                getcwd(diretorio, sizeof(diretorio));
                arqImagem = fopen("img.jpg", "wb");
                if (!arqImagem)
                {
                    perror("Erro ao abrir arquivo");
                    return 205;
                }

                printf("[%02d/%02d/%04d %02d:%02d:%02d]<%s> : Arquivo de midia enviado! O arquivo pode ser encontrado em <%s/img.jpg>\n",
                       tm_now->tm_mday, tm_now->tm_mon + 1, tm_now->tm_year + 1900,
                       tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec, getenv("USER"), diretorio);
                fprintf(log, "Midia Recebida: [%02d/%02d/%04d %02d:%02d:%02d]<%s> : Arquivo de midia enviado! O arquivo pode ser encontrado em <%s/img.jpg>\n\n",
                        tm_now->tm_mday, tm_now->tm_mon + 1, tm_now->tm_year + 1900,
                        tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec, getenv("USER"), diretorio);

                for (int i = 0; i < packAtual; i++)
                {
                    fwrite(mensagemCompleta[i], 1, sizeof(mensagemCompleta[i]), arqImagem);
                }
                fclose(arqImagem);
            }
            if (stateAuxiliar == RECEBE_MENSAGEM)
            {
                for (int i = 0; i < packAtual; i++)
                {
                    printf("%s", mensagemCompleta[i]);
                    fprintf(log, "Mensagem recebida: %s", mensagemCompleta[i]);
                }
            }
        }
        state = NADA_RECEBIDO;
    }
    fclose(log);
}