// --------
// IMPORTS
// --------

#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <limits.h> 
#include <math.h>

// -----------------------
// VERIFICACAO DO SISTEMA
// ----------------------

#ifdef WIN32
#include <windows.h>    // Apenas para Windows
#endif
#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#else
#include <GL/gl.h>     // Funções da OpenGL
#include <GL/glu.h>    // Funções da GLU
#include <GL/glut.h>   // Funções da FreeGLUT
#endif


// ---------------------------
// CHAMADA DA BIBLIOTECA SOIL
// ---------------------------

#include "SOIL.h"

// --------------------------------
// CHAMADA DA BIBLIOTECA DE FUNCOES
// --------------------------------

#include "functions-carving.c"

// ----------
// PROTOTIPOS
// ----------

void load(char* name, Img* pic);
void uploadTexture();

// -------------------------------------
// Funções da interface gráfica e OpenGL
// -------------------------------------

void init();
void draw();
void keyboard(unsigned char key, int x, int y);

// ------------------------
// INICIACAO DAS VARIAVEIS 
// -----------------------

// Largura e altura da janela
int width, height;

// Identificadores de textura
GLuint tex[3];

// As 3 imagens
Img pic[3];

// Imagem selecionada (0,1,2)
int sel;

// -------------------------------------
// Carrega uma imagem para a struct Img
// -------------------------------------

void load(char* name, Img* pic){
    int chan;
    pic->img = (RGB*) SOIL_load_image(name, &pic->width, &pic->height, &chan, SOIL_LOAD_RGB);
    if(!pic->img)
    {
        printf( "SOIL loading error: '%s'\n", SOIL_last_result() );
        exit(1);
    }
    printf("Load: %d x %d x %d\n", pic->width, pic->height, chan);
}

// --------------
// MAIN INTERFACE
// --------------

int main(int argc, char** argv){
    if(argc < 2) {
        printf("seamcarving [origem] [mascara]\n");
        printf("Origem é a imagem original, mascara é a máscara desejada\n");
        exit(1);
    }
    glutInit(&argc,argv);

    // Define do modo de operacao da GLUT
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

    // pic[0] -> imagem original
    // pic[1] -> máscara desejada
    // pic[2] -> resultado do algoritmo

    // Carrega as duas imagens
    load(argv[1], &pic[0]);
    load(argv[2], &pic[1]);

    if(pic[0].width != pic[1].width || pic[0].height != pic[1].height) {
        printf("Imagem e máscara com dimensões diferentes!\n");
        exit(1);
    }

    // A largura e altura da janela são calculadas de acordo com a maior
    // dimensão de cada imagem
    width = pic[0].width;
    height = pic[0].height;

    // A largura e altura da imagem de saída são iguais às da imagem original (1)
    pic[2].width  = pic[1].width;
    pic[2].height = pic[1].height;

    // Especifica o tamanho inicial em pixels da janela GLUT
    glutInitWindowSize(width, height);

    // Cria a janela passando como argumento o titulo da mesma
    glutCreateWindow("Seam Carving");

    // Registra a funcao callback de redesenho da janela de visualizacao
    glutDisplayFunc(draw);

    // Registra a funcao callback para tratamento das teclas ASCII
    glutKeyboardFunc (keyboard);

    // Cria texturas em memória a partir dos pixels das imagens
    tex[0] = SOIL_create_OGL_texture((unsigned char*) pic[0].img, pic[0].width, pic[0].height, SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);
    tex[1] = SOIL_create_OGL_texture((unsigned char*) pic[1].img, pic[1].width, pic[1].height, SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

    // Exibe as dimensões na tela, para conferência
    printf("Origem  : %s %d x %d\n", argv[1], pic[0].width, pic[0].height);
    printf("Destino : %s %d x %d\n", argv[2], pic[1].width, pic[0].height);
    sel = 0; // pic1

    // Define a janela de visualizacao 2D
    glMatrixMode(GL_PROJECTION);
    gluOrtho2D(0.0,width,height,0.0);
    glMatrixMode(GL_MODELVIEW);

    // Aloca memória para a imagem de saída
    pic[2].img = malloc(pic[1].width * pic[1].height * 3); // W x H x 3 bytes (RGB)
    // Pinta a imagem resultante de preto!
    memset(pic[2].img, 0, width*height*3);

    // Cria textura para a imagem de saída
    tex[2] = SOIL_create_OGL_texture((unsigned char*) pic[2].img, pic[2].width, pic[2].height, SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

    // Entra no loop de eventos, não retorna
    glutMainLoop();
}

// -------------------------------
// GERENCIA DE EVENTOS DO TECLADO
// -------------------------------

void keyboard(unsigned char key, int x, int y){
    if(key==27) {
      // ESC: libera memória e finaliza
      free(pic[0].img);
      free(pic[1].img);
      free(pic[2].img);
      exit(1);
    }
    if(key >= '1' && key <= '3')
        // 1-3: seleciona a imagem correspondente (origem, máscara e resultado)
        sel = key - '1';
    if(key == 's') {

        long g = 0;
        long i = 0;


        int matrixPixels[pic[0].height * pic[0].width];
        int frstColumnList[pic[0].height - 2];
        int lastColumnList[pic[0].height - 2];

        // Preenche um array de inteiros com os numeros referentes aos pixels da primeira coluna da imagem
        for(i; i < (pic[0].height - 2); i++){
            frstColumnList[i] = pic[0].width * (i+1);
        }
        // Preenche um array de inteiros com os numeros referentes aos pixels da ultima coluna da imagem
        int j = 1;
        for(i; i < (pic[0].height - 1); i++){
            lastColumnList[i] = (pic[0].width * (j+1)) -1;
            j++;
        }
        int count;
        int pixelsImportantes[pic[0].width]; // Pixels da linha escolhida vermelha
        int qtdP = sizeof(frstColumnList)/sizeof(int);
        int qtdU = sizeof(lastColumnList)/sizeof(int);

        printf("\n    Aplicando Algoritmo...\n\n");
        

        // USANDO O ALGORITMO
                    
        verificaEnergia(matrixPixels,frstColumnList,qtdP,lastColumnList,qtdU);
        int LinhaExcluir = identificaLinha(pixelsImportantes, &count);

        if(count != 0){
            verificaVermelho(matrixPixels, frstColumnList, qtdP , lastColumnList , qtdU , LinhaExcluir , pixelsImportantes , count);

        }else{
            pintaVerde(i);
            seamCarming(matrixPixels, frstColumnList , qtdP , lastColumnList , qtdU , i);
        }
            

                
            
        // Constroi a imagem no pic[2]
        for(i; i<(pic[2].height*pic[2].width); i++){
            pic[2].img[i].r = pic[0].img[i].r;
            pic[2].img[i].g = pic[0].img[i].g;
            pic[2].img[i].b = pic[0].img[i].b;

            // Coloca em Preto o que foi retirado
            if(pic[2].img[i].r == 0 && pic[2].img[i].g == 255 && pic[2].img[i].b == 0){
                pic[2].img[i].r = 0;
                pic[2].img[i].g = 0;
                pic[2].img[i].b = 0;
            }
        }

        uploadTexture();
        printf("\n");
        printf("\n\n Finalizado!!! \n");
    }
    glutPostRedisplay();
}

// ------------------------------
// UPLOAD E EXIBICAO DAS IMAGENS
// ------------------------------

void uploadTexture(){
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
        pic[2].width, pic[2].height, 0,
        GL_RGB, GL_UNSIGNED_BYTE, pic[2].img);
    glDisable(GL_TEXTURE_2D);
}


// -----------------------------
// CALLBACK DE REDESENHO NA TELA
// -----------------------------

void draw(){

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // Preto
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    // Para outras cores, veja exemplos em /etc/X11/rgb.txt

    glColor3ub(255, 255, 255);  // branco

    // Ativa a textura corresponde à imagem desejada
    glBindTexture(GL_TEXTURE_2D, tex[sel]);
    // E desenha um retângulo que ocupa toda a tela
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);

    glTexCoord2f(0,0);
    glVertex2f(0,0);

    glTexCoord2f(1,0);
    glVertex2f(pic[sel].width,0);

    glTexCoord2f(1,1);
    glVertex2f(pic[sel].width, pic[sel].height);

    glTexCoord2f(0,1);
    glVertex2f(0,pic[sel].height);

    glEnd();
    glDisable(GL_TEXTURE_2D);

    // Exibe a imagem
    glutSwapBuffers();
}
