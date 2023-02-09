#include <ncurses.h>
#include <string.h>

#define TAM 256



int main()
{
    int ch;

    char mensagem[TAM];

    int counter = 0;
    int i = 0;

    int opt[3];
    int indice = 0;

    memset(mensagem, 0, TAM);
    initscr();            // Inicializa o ncurses
    cbreak();             // Desativa o buffer de entrada
    keypad(stdscr, TRUE); // Ativa o suporte a teclas especiais
    timeout(1000);

    while (1)
    {
        ch = getch();
        if ((ch == 105) || (ch == 58))
            break;
        else
            clear();
    }
    clear();
    if (ch == 105)
    {
        printw("Insira sua mensagem: ( Enter para enviar )\n");
        // Enquanto o caractere recebido for diferente de enter
        while ((ch = getch()) != 10)
        {
            // timeout nao altera em nada
            if (ch != ERR)
            {
                // Difente da tecla de apaga caractere
                if ((ch == 263) && (i > 0))
                    i--;
                else
                {
                    mensagem[i] = ch;
                    i++;
                }
            }
            if (ch == 27)
            {
                memset(mensagem, 0, TAM);
                break;
            }
        }
    }
    if (ch == 58)
    {
        printw(":");
        // Enquanto o caractere recebido for diferente de enter
        while (1)
        {
            ch = getch();
            // timeout nao altera em nada
            if (ch != ERR)
            {
                // Difente da tecla de apaga caractere
                if ((ch == 263) && (i > 0))
                    i--;
                else
                {
                    mensagem[i] = ch;
                    i++;
                }
            }
            if (ch == 27)
            {
                memset(mensagem, 0, TAM);
                break;
            }

            if (ch == 10)
                break;
        }
    }
    clear();
    endwin(); // Finaliza o ncurses
    
    printf("%s", mensagem);



    // char q e char enter
    if (mensagem[0] == 113 && mensagem[1] == 10)
    {
        printf("Encerrando programa");
        return 0;
    }

    // send
    if ((mensagem[0] == 115) && (mensagem[1] == 101) && (mensagem[2] == 110) && (mensagem[3] == 100) && (mensagem[4] == 32))
    {
        printf("Enviando arquivo");
        return 0;
    }

    return 0;
}
