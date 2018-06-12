//     Universidade Federal do Rio Grande do Sul
//             Instituto de Informática
//       Departamento de Informática Aplicada
//
//    INF01047 Fundamentos de Computação Gráfica
//               Prof. Eduardo Gastal
//
//          TRABALHO FINAL - THE WALKING COW
// POR:
//      FELIPE ZORZO PEREIRA
//      WILLIAM WILBERT VARGAS

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
// Headers abaixo são específicos de C++
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>   // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h>  // Criação de janelas do sistema operacional

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Headers da biblioteca para carregar modelos obj
#include <tiny_obj_loader.h>

#include <stb_image.h>

// Headers locais, definidos na pasta "include/"
#include "utils.h"
#include "matrices.h"

//vai ser usado para calcular a velocidade angular
#define M_PI   3.14159265358979323846
//direções do movimento angular
#define UP 1
#define DOWN 2

#define MAX_ANGLE 0.13089966389//7.5°
#define MIN_ANGLE -0.26179938779 // -15°

#define GUN_RECOIL_ANGLE 2 * 0.01745329251//1º
#define BULLET_SPEED 50
#define MAX_DISTANCE_TO_BULLET 100

#define COW_LIFE 5
#define PLAYER_LIFE 3

#define MAX_NUM_OF_COWS 20

#define MAP_X_REPEAT 20 // repetições da textura no plano
#define MAP_Z_REPEAT 20

#define COW  1
#define PLANE 2
#define M4A1 3
#define CHAIR 4
#define BULLET 5

// Estrutura que representa um modelo geométrico carregado a partir de um
// arquivo ".obj". Veja https://en.wikipedia.org/wiki/Wavefront_.obj_file .
struct ObjModel
{
    tinyobj::attrib_t                 attrib;
    std::vector<tinyobj::shape_t>     shapes;
    std::vector<tinyobj::material_t>  materials;

    // Este construtor lê o modelo de um arquivo utilizando a biblioteca tinyobjloader.
    // Veja: https://github.com/syoyo/tinyobjloader
    ObjModel(const char* filename, const char* basepath = NULL, bool triangulate = true)
    {
        printf("Carregando modelo \"%s\"... ", filename);

        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename, basepath, triangulate);

        if (!err.empty())
            fprintf(stderr, "\n%s\n", err.c_str());

        if (!ret)
            throw std::runtime_error("Erro ao carregar modelo.");

        printf("OK.\n");
    }
};


// Declaração de funções utilizadas para pilha de matrizes de modelagem.
void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4& M);

// Declaração de várias funções utilizadas em main().  Essas estão definidas
// logo após a definição de main() neste arquivo.
void BuildTrianglesAndAddToVirtualScene(ObjModel*); // Constrói representação de um ObjModel como malha de triângulos para renderização
void ComputeNormals(ObjModel* model); // Computa normais de um ObjModel, caso não existam.
void LoadShadersFromFiles(); // Carrega os shaders de vértice e fragmento, criando um programa de GPU
void LoadTextureImage(const char* filename); // Função que carrega imagens de textura
void DrawVirtualObject(const char* object_name); // Desenha um objeto armazenado em g_VirtualScene
GLuint LoadShader_Vertex(const char* filename);   // Carrega um vertex shader
GLuint LoadShader_Fragment(const char* filename); // Carrega um fragment shader
void LoadShader(const char* filename, GLuint shader_id); // Função utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id); // Cria um programa de GPU
void PrintObjModelInfo(ObjModel*); // Função para debugging

// Declaração de funções auxiliares para renderizar texto dentro da janela
// OpenGL. Estas funções estão definidas no arquivo "textrendering.cpp".
void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void TextRendering_PrintString(GLFWwindow* window, const std::string &str, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrix(GLFWwindow* window, glm::mat4 M, float x, float y, float scale = 1.0f);
void TextRendering_PrintVector(GLFWwindow* window, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProduct(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductDivW(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);

// Funções abaixo renderizam como texto na janela OpenGL algumas matrizes e
// outras informações do programa. Definidas após main().
void TextRendering_ShowModelViewProjection(GLFWwindow* window, glm::mat4 projection, glm::mat4 view, glm::mat4 model, glm::vec4 p_model);
void TextRendering_ShowEulerAngles(GLFWwindow* window);
void TextRendering_ShowProjection(GLFWwindow* window);
void TextRendering_ShowFramesPerSecond(GLFWwindow* window);

// Funções callback para comunicação com o sistema operacional e interação do
// usuário. Veja mais comentários nas definições das mesmas, abaixo.
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

//Funções para manipulação da câmera tanto movimento da posição quando do
//vetor view
void setCameraPosition(float x, float y, float z);
glm::vec3 toSpherical(float x, float y, float z);

void moveForward();
void moveBack();
void moveLeft();
void moveRight();

void handleMovement();

//retorna um id da vaca que poderá ser usado para deletá-la posteriormente
int createCow(double xpos, double zpos);
void removeCow(int id);
void updateCows();
void drawCows();


int createBullet();
void removeBullet(int id);
void updateBullet();
void checkCollisionWithCows(glm::vec4 prevPos, glm::vec4 currentPos);
void drawBullet();

void updateRecoil();
void shoot();

// Definimos uma estrutura que armazenará dados necessários para renderizar
// cada objeto da cena virtual.
struct SceneObject
{
    std::string  name;        // Nome do objeto
    void*        first_index; // Índice do primeiro vértice dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    int          num_indices; // Número de índices do objeto dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    GLenum       rendering_mode; // Modo de rasterização (GL_TRIANGLES, GL_TRIANGLE_STRIP, etc.)
    GLuint       vertex_array_object_id; // ID do VAO onde estão armazenados os atributos do modelo
    glm::vec3    bbox_min; // Axis-Aligned Bounding Box do objeto
    glm::vec3    bbox_max;
};

// Abaixo definimos variáveis globais utilizadas em várias funções do código.

// A cena virtual é uma lista de objetos nomeados, guardados em um dicionário
// (map).  Veja dentro da função BuildTrianglesAndAddToVirtualScene() como que são incluídos
// objetos dentro da variável g_VirtualScene, e veja na função main() como
// estes são acessados.
std::map<std::string, SceneObject> g_VirtualScene;

// Pilha que guardará as matrizes de modelagem.
std::stack<glm::mat4>  g_MatrixStack;

// Razão de proporção da janela (largura/altura). Veja função FramebufferSizeCallback().
float g_ScreenRatio = 1.0f;

// Ângulos de Euler que controlam a rotação de um dos cubos da cena virtual
float g_AngleX = 0.0f;
float g_AngleY = 0.0f;
float g_AngleZ = 0.0f;

// "g_LeftMouseButtonPressed = true" se o usuário está com o botão esquerdo do mouse
// pressionado no momento atual. Veja função MouseButtonCallback().
bool g_LeftMouseButtonPressed = false;
bool g_RightMouseButtonPressed = false; // Análogo para botão direito do mouse
bool g_MiddleMouseButtonPressed = false; // Análogo para botão do meio do mouse

// Variáveis que definem a câmera em coordenadas esféricas, controladas pelo
// usuário através do mouse (veja função CursorPosCallback()). A posição
// efetiva da câmera é calculada dentro da função main(), dentro do loop de
// renderização.
float g_CameraTheta = 0.0f; // Ângulo no plano ZX em relação ao eixo Z
float g_CameraPhi = 0.0f;   // Ângulo em relação ao eixo Y
float g_CameraRo = 0.0f;   // Ângulo em relação ao eixo Y
float g_CameraDistance = 3.5f; // Distância da câmera para a origem
// Variáveis que controlam rotação do antebraço
float g_ForearmAngleZ = 0.0f;
float g_ForearmAngleX = 0.0f;

// Variáveis que controlam translação do torso
float g_TorsoPositionX = 0.0f;
float g_TorsoPositionY = 0.0f;

// Variável que controla o tipo de projeção utilizada: perspectiva ou ortográfica.
bool g_UsePerspectiveProjection = true;

// Variável que controla se o texto informativo será mostrado na tela.
bool g_ShowInfoText = true;

// Variáveis que definem um programa de GPU (shaders). Veja função LoadShadersFromFiles().
GLuint vertex_shader_id;
GLuint fragment_shader_id;
GLuint program_id = 0;
GLint model_uniform;
GLint view_uniform;
GLint projection_uniform;
GLint object_id_uniform;
GLint bbox_min_uniform;
GLint bbox_max_uniform;

// Número de texturas carregadas pela função LoadTextureImage()
GLuint g_NumLoadedTextures = 0;

struct FreeCamera{
    glm::vec4 camera_position;
    glm::vec4 camera_view;
    glm::vec4 camera_up;
};

typedef struct cowStruct{
    int id;
    double xpos;
    double zpos;
    double angle;
    int angularMovementDirection;
}Cow;

typedef struct cowList CowList;

struct cowList{
    Cow* currentCow;
    CowList* next;
};


typedef struct bulletStruct{
    int id;
    glm::vec4 pos;
    glm::vec4 direction;
}Bullet;

typedef struct bulletList BulletList;

struct bulletList{
        Bullet* currentBullet;
        BulletList* next;
};


//verifica se um segmento de reta de prevPos até currentPos se choca com a bounding box da vaca
bool boundingBoxCollided(Cow* cow, glm::vec4 pos);
//verifica se um segmento de reta de prevPos até currentPos se choca com a vaca verificando de forma mais precisa que com a bounding box
bool fineCollision(Cow* cow, glm::vec4 pos);
double cowAngleToCamera(double xpos, double zpos);

//velocidade de rotação da vaca andando
float cowAngularSpeed = M_PI /2.0; // 90° per second
float balanceSpeed = 7*M_PI;
float recoilSpeed = M_PI/2;
float currentRecoilAngle = 0.0f;
int recoilDirection = UP;
float shotDelay = 0.1f; // em s
float currentShotDelay = 0.0f;
bool onRecoil = false;

float cowGenerationDelay = 1.0f; //s
float currentCowDelay = 0.0f;
float cowSpeed = 2.5;
float minDistanceToCow = 1.0f;

int currentLivingCows = 0;
double ellapsed_time = 0;

int angularMovementDirection = UP;
bool isMoving = false;
int isWPressed = false;
int isAPressed = false;
int isSPressed = false;
int isDPressed = false;

float movementSpeed = 3.0f;
//velocidade de rotação da câmera ao clicar e arrastar com o mouse
float cameraRotationSpeed = 0.003f;
float cameraBaseHeight = 1.0f;
float walkHeightAmplitude = 0.05f;
const glm::vec4 initialCameraPos = glm::vec4(2.0f, 1.0f, 2.0f, 1.0f);
const glm::vec4 initialCameraView = glm::vec4(-2.0f, -1.0f, -2.0f, 0.0f)/norm(glm::vec4(-2.0f, -1.0f, -2.0f, 0.0f));
const glm::vec4 cameraUpVec = glm::vec4(0.0f, 1.0f, 0.0f, 0.0);

float yRotation = 0.0f;
float xRotation = 0.0f;
float zRotation = 0.0f;

int currentCowId = 1;
int currentBulletId = 1;

const glm::vec4 baseWeaponVector = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);


CowList* cows = NULL;
BulletList* bullets = NULL;


FreeCamera Camera;

int debug = true;

int main(int argc, char* argv[])
{
    // Inicializamos a biblioteca GLFW, utilizada para criar uma janela do
    // sistema operacional, onde poderemos renderizar com OpenGL.
    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }
    //inicializa a camera
    Camera.camera_position = initialCameraPos;
    Camera.camera_view = initialCameraView;
    Camera.camera_up = cameraUpVec;

    // Definimos o callback para impressão de erros da GLFW no terminal
    glfwSetErrorCallback(ErrorCallback);
    // Pedimos para utilizar OpenGL versão 3.3 (ou superior)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    // Pedimos para utilizar o perfil "core", isto é, utilizaremos somente as
    // funções modernas de OpenGL.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Criamos uma janela do sistema operacional, com 800 colunas e 600 linhas
    // de pixels, e com título "INF01047 ...".
    GLFWwindow* window;
    window = glfwCreateWindow(800, 600, "INF01047 - The Walking Cow - Felipe Zorzo Pereira e William Wilbert Vargas", NULL/*glfwGetPrimaryMonitor()*/, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos a função de callback que será chamada sempre que o usuário
    // pressionar alguma tecla do teclado ...
    glfwSetKeyCallback(window, KeyCallback);
    // ... ou clicar os botões do mouse ...
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    // ... ou movimentar o cursor do mouse em cima da janela ...
    glfwSetCursorPosCallback(window, CursorPosCallback);
    // ... ou rolar a "rodinha" do mouse.
    glfwSetScrollCallback(window, ScrollCallback);
    //O cursor do mouse é posto em modo escondido
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Indicamos que as chamadas OpenGL deverão renderizar nesta janela
    glfwMakeContextCurrent(window);

    // Carregamento de todas funções definidas por OpenGL 3.3, utilizando a
    // biblioteca GLAD.
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    // Definimos a função de callback que será chamada sempre que a janela for
    // redimensionada, por consequência alterando o tamanho do "framebuffer"
    // (região de memória onde são armazenados os pixels da imagem).
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    FramebufferSizeCallback(window, 800, 600); // Forçamos a chamada do callback acima, para definir g_ScreenRatio.

    // Imprimimos no terminal informações sobre a GPU do sistema
    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    // Carregamos os shaders de vértices e de fragmentos que serão utilizados
    // para renderização. Veja slide 217 e 219 do documento no Moodle
    // "Aula_03_Rendering_Pipeline_Grafico.pdf".
    //
    LoadShadersFromFiles();

    // Carregamos duas imagens para serem utilizadas como textura
    LoadTextureImage("../../data/cow_texture.jpg");      // TextureImage0
    LoadTextureImage("../../data/grass_texture.jpg");
    LoadTextureImage("../../data/gun_texture.jpg");
    LoadTextureImage("../../data/yellowTexture.jpg");


    ObjModel cowmodel("../../data/cow.obj");
    ComputeNormals(&cowmodel);
    BuildTrianglesAndAddToVirtualScene(&cowmodel);

    ObjModel planemodel("../../data/plane.obj");
    ComputeNormals(&planemodel);
    BuildTrianglesAndAddToVirtualScene(&planemodel);

    ObjModel m4a1model("../../data/m4a1/m4a1.obj");
    ComputeNormals(&m4a1model);
    BuildTrianglesAndAddToVirtualScene(&m4a1model);

    ObjModel chairmodel("../../data/chair1.obj");
    ComputeNormals(&chairmodel);
    BuildTrianglesAndAddToVirtualScene(&chairmodel);

    ObjModel bulletmodel("../../data/bullet.obj");
    ComputeNormals(&bulletmodel);
    BuildTrianglesAndAddToVirtualScene(&bulletmodel);

    if ( argc > 1 )
    {
        ObjModel model(argv[1]);
        BuildTrianglesAndAddToVirtualScene(&model);
    }

    // Inicializamos o código para renderização de texto.
    TextRendering_Init();

    // Habilitamos o Z-buffer. Veja slide 66 do documento "Aula_13_Clipping_and_Culling.pdf".
    glEnable(GL_DEPTH_TEST);

    // Habilitamos o Backface Culling. Veja slides 22 à 34 do documento "Aula_13_Clipping_and_Culling.pdf".
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Variáveis auxiliares utilizadas para chamada à função
    // TextRendering_ShowModelViewProjection(), armazenando matrizes 4x4.
    glm::mat4 the_projection;
    glm::mat4 the_model;
    glm::mat4 the_view;

    // Ficamos em loop, renderizando, até que o usuário feche a janela

    double prev_time = glfwGetTime();
    double prev_cameraBalAngle = 0.0;

    //model = Matrix_Identity(); // Transformação identidade de modelagem
    //model = model * Matrix_Translate(initialCameraPos.x+0.5, initialCameraPos.y-1.0f, initialCameraPos.z-0.5f);
    //model = Matrix_Rotate_Y(150);
    float teste = 0.1;


    while (!glfwWindowShouldClose(window))
    {
        FramebufferSizeCallback(window, 800, 600);
        // Aqui executamos as operações de renderização

        // Definimos a cor do "fundo" do framebuffer como branco.  Tal cor é
        // definida como coeficientes RGBA: Red, Green, Blue, Alpha; isto é:
        // Vermelho, Verde, Azul, Alpha (valor de transparência).
        // Conversaremos sobre sistemas de cores nas aulas de Modelos de Iluminação.
        //
        //           R     G     B     A
        glClearColor(0.1f, 0.2f, 0.0f, 1.0f);

        // "Pintamos" todos os pixels do framebuffer com a cor definida acima,
        // e também resetamos todos os pixels do Z-buffer (depth buffer).
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Pedimos para a GPU utilizar o programa de GPU criado acima (contendo
        // os shaders de vértice e fragmentos).
        glUseProgram(program_id);

        // Computamos a posição da câmera utilizando coordenadas esféricas.  As
        // variáveis g_CameraDistance, g_CameraPhi, e g_CameraTheta são
        // controladas pelo mouse do usuário. Veja as funções CursorPosCallback()
        // e ScrollCallback().

        // Computamos a matriz "View" utilizando os parâmetros da câmera para
        // definir o sistema de coordenadas da câmera.  Veja slide 179 do
        // documento "Aula_08_Sistemas_de_Coordenadas.pdf".
        glm::mat4 view = Matrix_Camera_View(Camera.camera_position,
                                            Camera.camera_view,
                                            Camera.camera_up);
        // Agora computamos a matriz de Projeção.
        glm::mat4 projection;

        // Note que, no sistema de coordenadas da câmera, os planos near e far
        // estão no sentido negativo! Veja slides 191-194 do documento
        // "Aula_09_Projecoes.pdf".
        float nearplane = -0.1f;  // Posição do "near plane"
        float farplane  = -40.0f; // Posição do "far plane"

        if (g_UsePerspectiveProjection)
        {
            // Projeção Perspectiva.
            // Para definição do field of view (FOV), veja slide 228 do
            // documento "Aula_09_Projecoes.pdf".
            float field_of_view = 3.141592 / 3.0f;
            projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);
        }
        else
        {
            // Projeção Ortográfica.
            // Para definição dos valores l, r, b, t ("left", "right", "bottom", "top"),
            // veja slide 243 do documento "Aula_09_Projecoes.pdf".
            // Para simular um "zoom" ortográfico, computamos o valor de "t"
            // utilizando a variável g_CameraDistance.
            float cameraDistance = sqrt(Camera.camera_position.x * Camera.camera_position.x +
                                        Camera.camera_position.y * Camera.camera_position.y +
                                        Camera.camera_position.z * Camera.camera_position.z);
            float t = 1.5f*cameraDistance/2.5f;
            float b = -t;
            float r = t*g_ScreenRatio;
            float l = -r;
            projection = Matrix_Orthographic(l, r, b, t, nearplane, farplane);
        }


        // Enviamos as matrizes "view" e "projection" para a placa de vídeo
        // (GPU). Veja o arquivo "shader_vertex.glsl", onde estas são
        // efetivamente aplicadas em todos os pontos.
        glUniformMatrix4fv(view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
        glUniformMatrix4fv(projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));



        double current_time = glfwGetTime();
        ellapsed_time = current_time - prev_time;

        if (currentShotDelay > 0){
            currentShotDelay -= ellapsed_time;
        }
        if (onRecoil){
                updateRecoil();
        }
        if (g_LeftMouseButtonPressed){
            if(currentShotDelay <= 0){
                shoot();
            }
        }
        srand(time(NULL));
        if(currentCowDelay > 0){
            currentCowDelay -= ellapsed_time;

        }
        if(currentCowDelay <= 0 && currentLivingCows + 1 <= MAX_NUM_OF_COWS){
            createCow(MAP_X_REPEAT/2 - 1 - rand()%(MAP_X_REPEAT - 2), MAP_Z_REPEAT/2 - 1 - rand()%(MAP_Z_REPEAT - 2)); // subtrai-se 2 para evitar que a vaca fique na parte de fora da borda do mapa
            currentCowDelay = cowGenerationDelay;
        }

        updateCows();
        drawCows();
        updateBullet();
        drawBullet();

        glm::mat4 model =
                  Matrix_Translate(Camera.camera_position.x, Camera.camera_position.y, Camera.camera_position.z)
                  * Matrix_Rotate_Y(3.92 + yRotation)  //depois a rotacionamos horizontalmente
                  * Matrix_Rotate_X(-Camera.camera_view.y - currentRecoilAngle)
                  * Matrix_Scale(0.05f, 0.05f, 0.05f); //primeiro escalamos a arma
        //printf("x: %f\ty: %f\tz: %f\n", xRotation, yRotation, zRotation);
        //printf("V : %f\t%f\t%f\n", HorizCrossProduct.x, HorizCrossProduct.y, HorizCrossProduct.z);
        //printf("cameraphi : %f\n", g_CameraPhi);
        //model = model * Matrix_Rotate_X(1+ g_CameraPhi, Horiz);
        //printf("%f\n", Camera.camera_view.y);
        model = model * Matrix_Translate(-10.0f, -18.0f, -4.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(object_id_uniform, M4A1);
        DrawVirtualObject("weapon");

        glm::vec4 viewPoint = Camera.camera_position + Camera.camera_view;
        model = Matrix_Identity();

        teste += 0.1;
        model = model
                    * Matrix_Translate(viewPoint.x, viewPoint.y, viewPoint.z)
                    //* Matrix_Translate(Camera.camera_position.x-1.0f, Camera.camera_position.y, Camera.camera_position.z)
                    * Matrix_Rotate_Y(0.809+yRotation)
                    * Matrix_Rotate_X(Camera.camera_view.y)
                    * Matrix_Scale(0.01f, 0.01f, 0.01f);
        model = model * Matrix_Rotate_X(g_CameraPhi);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(object_id_uniform, CHAIR);
        DrawVirtualObject("Plane2");

        //desenho do chão
        model = Matrix_Translate(0.0f, -0.64f, 0.0f) *
                Matrix_Scale(20.0f, 1.0f, 20.0f);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, PLANE);
        //faz o chão repetir espelhadamente para não notar transições
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        DrawVirtualObject("plane");

        double cameraBalanceAngle;
        if (isMoving){
            //Balanço do movimento
            cameraBalanceAngle = prev_cameraBalAngle + ellapsed_time * balanceSpeed;
            if (cameraBalanceAngle > 2* M_PI){
                cameraBalanceAngle -=2*M_PI;
            }
            setCameraPosition(
                          Camera.camera_position.x,
                          1.0f + walkHeightAmplitude * sin(cameraBalanceAngle),
                          Camera.camera_position.z
                          );
        }
        else{
            cameraBalanceAngle = 0;
        }


        prev_cameraBalAngle = cameraBalanceAngle;
        prev_time = current_time;




        // Pegamos um vértice com coordenadas de modelo (0.5, 0.5, 0.5, 1) e o
        // passamos por todos os sistemas de coordenadas armazenados nas
        // matrizes the_model, the_view, e the_projection; e escrevemos na tela
        // as matrizes e pontos resultantes dessas transformações.

        // Imprimimos na tela os ângulos de Euler que controlam a rotação do
        // terceiro cubo.
        TextRendering_ShowEulerAngles(window);

        // Imprimimos na informação sobre a matriz de projeção sendo utilizada.
        TextRendering_ShowProjection(window);

        // Imprimimos na tela informação sobre o número de quadros renderizados
        // por segundo (frames per second).
        TextRendering_ShowFramesPerSecond(window);

        // O framebuffer onde OpenGL executa as operações de renderização não
        // é o mesmo que está sendo mostrado para o usuário, caso contrário
        // seria possível ver artefatos conhecidos como "screen tearing". A
        // chamada abaixo faz a troca dos buffers, mostrando para o usuário
        // tudo que foi renderizado pelas funções acima.
        // Veja o link: Veja o link: https://en.wikipedia.org/w/index.php?title=Multiple_buffering&oldid=793452829#Double_buffering_in_computer_graphics
        glfwSwapBuffers(window);

        // Verificamos com o sistema operacional se houve alguma interação do
        // usuário (teclado, mouse, ...). Caso positivo, as funções de callback
        // definidas anteriormente usando glfwSet*Callback() serão chamadas
        // pela biblioteca GLFW.
        glfwPollEvents();

        if(isMoving)
            handleMovement();
    }

    // Finalizamos o uso dos recursos do sistema operacional
    glfwTerminate();

    // Fim do programa
    return 0;
}

// Função que carrega uma imagem para ser utilizada como textura
void LoadTextureImage(const char* filename)
{
    printf("Carregando imagem \"%s\"... ", filename);

    // Primeiro fazemos a leitura da imagem do disco
    stbi_set_flip_vertically_on_load(true);
    int width;
    int height;
    int channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 3);

    if ( data == NULL )
    {
        fprintf(stderr, "ERROR: Cannot open image file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }

    printf("OK (%dx%d).\n", width, height);

    // Agora criamos objetos na GPU com OpenGL para armazenar a textura
    GLuint texture_id;
    GLuint sampler_id;
    glGenTextures(1, &texture_id);
    glGenSamplers(1, &sampler_id);

    // Veja slide 160 do documento "Aula_20_e_21_Mapeamento_de_Texturas.pdf"
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Parâmetros de amostragem da textura. Falaremos sobre eles em uma próxima aula.
    glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Agora enviamos a imagem lida do disco para a GPU
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    GLuint textureunit = g_NumLoadedTextures;
    glActiveTexture(GL_TEXTURE0 + textureunit);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindSampler(textureunit, sampler_id);

    stbi_image_free(data);

    g_NumLoadedTextures += 1;
}

// Função que desenha um objeto armazenado em g_VirtualScene. Veja definição
// dos objetos na função BuildTrianglesAndAddToVirtualScene().
void DrawVirtualObject(const char* object_name)
{
    // "Ligamos" o VAO. Informamos que queremos utilizar os atributos de
    // vértices apontados pelo VAO criado pela função BuildTrianglesAndAddToVirtualScene(). Veja
    // comentários detalhados dentro da definição de BuildTrianglesAndAddToVirtualScene().
    glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);

    // Setamos as variáveis "bbox_min" e "bbox_max" do fragment shader
    // com os parâmetros da axis-aligned bounding box (AABB) do modelo.
    glm::vec3 bbox_min = g_VirtualScene[object_name].bbox_min;
    glm::vec3 bbox_max = g_VirtualScene[object_name].bbox_max;
    glUniform4f(bbox_min_uniform, bbox_min.x, bbox_min.y, bbox_min.z, 1.0f);
    glUniform4f(bbox_max_uniform, bbox_max.x, bbox_max.y, bbox_max.z, 1.0f);

    // Pedimos para a GPU rasterizar os vértices dos eixos XYZ
    // apontados pelo VAO como linhas. Veja a definição de
    // g_VirtualScene[""] dentro da função BuildTrianglesAndAddToVirtualScene(), e veja
    // a documentação da função glDrawElements() em
    // http://docs.gl/gl3/glDrawElements.
    glDrawElements(
        g_VirtualScene[object_name].rendering_mode,
        g_VirtualScene[object_name].num_indices,
        GL_UNSIGNED_INT,
        (void*)g_VirtualScene[object_name].first_index
    );

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Função que carrega os shaders de vértices e de fragmentos que serão
// utilizados para renderização. Veja slide 217 e 219 do documento
// "Aula_03_Rendering_Pipeline_Grafico.pdf".
//
void LoadShadersFromFiles()
{
    // Note que o caminho para os arquivos "shader_vertex.glsl" e
    // "shader_fragment.glsl" estão fixados, sendo que assumimos a existência
    // da seguinte estrutura no sistema de arquivos:
    //
    //    + FCG_Lab_01/
    //    |
    //    +--+ bin/
    //    |  |
    //    |  +--+ Release/  (ou Debug/ ou Linux/)
    //    |     |
    //    |     o-- main.exe
    //    |
    //    +--+ src/
    //       |
    //       o-- shader_vertex.glsl
    //       |
    //       o-- shader_fragment.glsl
    //
    vertex_shader_id = LoadShader_Vertex("../../src/shader_vertex.glsl");
    fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

    // Deletamos o programa de GPU anterior, caso ele exista.
    if ( program_id != 0 )
        glDeleteProgram(program_id);

    // Criamos um programa de GPU utilizando os shaders carregados acima.
    program_id = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    // Buscamos o endereço das variáveis definidas dentro do Vertex Shader.
    // Utilizaremos estas variáveis para enviar dados para a placa de vídeo
    // (GPU)! Veja arquivo "shader_vertex.glsl" e "shader_fragment.glsl".
    model_uniform           = glGetUniformLocation(program_id, "model"); // Variável da matriz "model"
    view_uniform            = glGetUniformLocation(program_id, "view"); // Variável da matriz "view" em shader_vertex.glsl
    projection_uniform      = glGetUniformLocation(program_id, "projection"); // Variável da matriz "projection" em shader_vertex.glsl
    object_id_uniform       = glGetUniformLocation(program_id, "object_id"); // Variável "object_id" em shader_fragment.glsl
    bbox_min_uniform        = glGetUniformLocation(program_id, "bbox_min");
    bbox_max_uniform        = glGetUniformLocation(program_id, "bbox_max");

    // Variáveis em "shader_fragment.glsl" para acesso das imagens de textura
    glUseProgram(program_id);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage0"), 0);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage1"), 1);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage2"), 2);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage3"), 3);
    glUseProgram(0);
}

// Função que pega a matriz M e guarda a mesma no topo da pilha
void PushMatrix(glm::mat4 M)
{
    g_MatrixStack.push(M);
}

// Função que remove a matriz atualmente no topo da pilha e armazena a mesma na variável M
void PopMatrix(glm::mat4& M)
{
    if ( g_MatrixStack.empty() )
    {
        M = Matrix_Identity();
    }
    else
    {
        M = g_MatrixStack.top();
        g_MatrixStack.pop();
    }
}

// Função que computa as normais de um ObjModel, caso elas não tenham sido
// especificadas dentro do arquivo ".obj"
void ComputeNormals(ObjModel* model)
{
    if ( !model->attrib.normals.empty() )
        return;

    // Primeiro computamos as normais para todos os TRIÂNGULOS.
    // Segundo, computamos as normais dos VÉRTICES através do método proposto
    // por Gourad, onde a normal de cada vértice vai ser a média das normais de
    // todas as faces que compartilham este vértice.

    size_t num_vertices = model->attrib.vertices.size() / 3;

    std::vector<int> num_triangles_per_vertex(num_vertices, 0);
    std::vector<glm::vec4> vertex_normals(num_vertices, glm::vec4(0.0f,0.0f,0.0f,0.0f));

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            glm::vec4  vertices[3];
            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                vertices[vertex] = glm::vec4(vx,vy,vz,1.0);
            }

            const glm::vec4  a = vertices[0];
            const glm::vec4  b = vertices[1];
            const glm::vec4  c = vertices[2];

            const glm::vec4  n = crossproduct(b-a,c-a);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                num_triangles_per_vertex[idx.vertex_index] += 1;
                vertex_normals[idx.vertex_index] += n;
                model->shapes[shape].mesh.indices[3*triangle + vertex].normal_index = idx.vertex_index;
            }
        }
    }

    model->attrib.normals.resize( 3*num_vertices );

    for (size_t i = 0; i < vertex_normals.size(); ++i)
    {
        glm::vec4 n = vertex_normals[i] / (float)num_triangles_per_vertex[i];
        n /= norm(n);
        model->attrib.normals[3*i + 0] = n.x;
        model->attrib.normals[3*i + 1] = n.y;
        model->attrib.normals[3*i + 2] = n.z;
    }
}

// Constrói triângulos para futura renderização a partir de um ObjModel.
void BuildTrianglesAndAddToVirtualScene(ObjModel* model)
{
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    std::vector<GLuint> indices;
    std::vector<float>  model_coefficients;
    std::vector<float>  normal_coefficients;
    std::vector<float>  texture_coefficients;

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t first_index = indices.size();
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        const float minval = std::numeric_limits<float>::min();
        const float maxval = std::numeric_limits<float>::max();

        glm::vec3 bbox_min = glm::vec3(maxval,maxval,maxval);
        glm::vec3 bbox_max = glm::vec3(minval,minval,minval);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];

                indices.push_back(first_index + 3*triangle + vertex);

                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                //printf("tri %d vert %d = (%.2f, %.2f, %.2f)\n", (int)triangle, (int)vertex, vx, vy, vz);
                model_coefficients.push_back( vx ); // X
                model_coefficients.push_back( vy ); // Y
                model_coefficients.push_back( vz ); // Z
                model_coefficients.push_back( 1.0f ); // W

                bbox_min.x = std::min(bbox_min.x, vx);
                bbox_min.y = std::min(bbox_min.y, vy);
                bbox_min.z = std::min(bbox_min.z, vz);
                bbox_max.x = std::max(bbox_max.x, vx);
                bbox_max.y = std::max(bbox_max.y, vy);
                bbox_max.z = std::max(bbox_max.z, vz);

                // Inspecionando o código da tinyobjloader, o aluno Bernardo
                // Sulzbach (2017/1) apontou que a maneira correta de testar se
                // existem normais e coordenadas de textura no ObjModel é
                // comparando se o índice retornado é -1. Fazemos isso abaixo.

                if ( idx.normal_index != -1 )
                {
                    const float nx = model->attrib.normals[3*idx.normal_index + 0];
                    const float ny = model->attrib.normals[3*idx.normal_index + 1];
                    const float nz = model->attrib.normals[3*idx.normal_index + 2];
                    normal_coefficients.push_back( nx ); // X
                    normal_coefficients.push_back( ny ); // Y
                    normal_coefficients.push_back( nz ); // Z
                    normal_coefficients.push_back( 0.0f ); // W
                }

                if ( idx.texcoord_index != -1 )
                {
                    const float u = model->attrib.texcoords[2*idx.texcoord_index + 0];
                    const float v = model->attrib.texcoords[2*idx.texcoord_index + 1];
                    texture_coefficients.push_back( u );
                    texture_coefficients.push_back( v );
                }
            }
        }

        size_t last_index = indices.size() - 1;

        SceneObject theobject;
        theobject.name           = model->shapes[shape].name;
        theobject.first_index    = (void*)first_index; // Primeiro índice
        theobject.num_indices    = last_index - first_index + 1; // Número de indices
        theobject.rendering_mode = GL_TRIANGLES;       // Índices correspondem ao tipo de rasterização GL_TRIANGLES.
        theobject.vertex_array_object_id = vertex_array_object_id;

        theobject.bbox_min = bbox_min;
        theobject.bbox_max = bbox_max;

        g_VirtualScene[model->shapes[shape].name] = theobject;
    }

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
    GLuint location = 0; // "(location = 0)" em "shader_vertex.glsl"
    GLint  number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if ( !normal_coefficients.empty() )
    {
        GLuint VBO_normal_coefficients_id;
        glGenBuffers(1, &VBO_normal_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
        location = 1; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if ( !texture_coefficients.empty() )
    {
        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
        location = 2; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 2; // vec2 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    GLuint indices_id;
    glGenBuffers(1, &indices_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // XXX Errado!

    glBindVertexArray(0);
}

// Carrega um Vertex Shader de um arquivo GLSL. Veja definição de LoadShader() abaixo.
GLuint LoadShader_Vertex(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos vértices.
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, vertex_shader_id);

    // Retorna o ID gerado acima
    return vertex_shader_id;
}

// Carrega um Fragment Shader de um arquivo GLSL . Veja definição de LoadShader() abaixo.
GLuint LoadShader_Fragment(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos fragmentos.
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, fragment_shader_id);

    // Retorna o ID gerado acima
    return fragment_shader_id;
}

// Função auxilar, utilizada pelas duas funções acima. Carrega código de GPU de
// um arquivo GLSL e faz sua compilação.
void LoadShader(const char* filename, GLuint shader_id)
{
    // Lemos o arquivo de texto indicado pela variável "filename"
    // e colocamos seu conteúdo em memória, apontado pela variável
    // "shader_string".
    std::ifstream file;
    try {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    } catch ( std::exception& e ) {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar* shader_string = str.c_str();
    const GLint   shader_string_length = static_cast<GLint>( str.length() );

    // Define o código do shader GLSL, contido na string "shader_string"
    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);

    // Compila o código do shader GLSL (em tempo de execução)
    glCompileShader(shader_id);

    // Verificamos se ocorreu algum erro ou "warning" durante a compilação
    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

    // Alocamos memória para guardar o log de compilação.
    // A chamada "new" em C++ é equivalente ao "malloc()" do C.
    GLchar* log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    // Imprime no terminal qualquer erro ou "warning" de compilação
    if ( log_length != 0 )
    {
        std::string  output;

        if ( !compiled_ok )
        {
            output += "ERROR: OpenGL compilation of \"";
            output += filename;
            output += "\" failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }
        else
        {
            output += "WARNING: OpenGL compilation of \"";
            output += filename;
            output += "\".\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }

        fprintf(stderr, "%s", output.c_str());
    }

    // A chamada "delete" em C++ é equivalente ao "free()" do C
    delete [] log;
}

// Esta função cria um programa de GPU, o qual contém obrigatoriamente um
// Vertex Shader e um Fragment Shader.
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
    // Criamos um identificador (ID) para este programa de GPU
    GLuint program_id = glCreateProgram();

    // Definição dos dois shaders GLSL que devem ser executados pelo programa
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    // Linkagem dos shaders acima ao programa
    glLinkProgram(program_id);

    // Verificamos se ocorreu algum erro durante a linkagem
    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    // Imprime no terminal qualquer erro de linkagem
    if ( linked_ok == GL_FALSE )
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        // Alocamos memória para guardar o log de compilação.
        // A chamada "new" em C++ é equivalente ao "malloc()" do C.
        GLchar* log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        // A chamada "delete" em C++ é equivalente ao "free()" do C
        delete [] log;

        fprintf(stderr, "%s", output.c_str());
    }

    // Os "Shader Objects" podem ser marcados para deleção após serem linkados
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    // Retornamos o ID gerado acima
    return program_id;
}

// Definição da função que será chamada sempre que a janela do sistema
// operacional for redimensionada, por consequência alterando o tamanho do
// "framebuffer" (região de memória onde são armazenados os pixels da imagem).
void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // Indicamos que queremos renderizar em toda região do framebuffer. A
    // função "glViewport" define o mapeamento das "normalized device
    // coordinates" (NDC) para "pixel coordinates".  Essa é a operação de
    // "Screen Mapping" ou "Viewport Mapping" vista em aula (slides 33 até 42
    // do documento "Aula_07_Transformacoes_Geometricas_3D.pdf").
    glViewport(0, 0, width, height);

    // Atualizamos também a razão que define a proporção da janela (largura /
    // altura), a qual será utilizada na definição das matrizes de projeção,
    // tal que não ocorra distorções durante o processo de "Screen Mapping"
    // acima, quando NDC é mapeado para coordenadas de pixels. Veja slide 228
    // do documento "Aula_09_Projecoes.pdf".
    //
    // O cast para float é necessário pois números inteiros são arredondados ao
    // serem divididos!
    g_ScreenRatio = (float)width / height;
}

// Variáveis globais que armazenam a última posição do cursor do mouse, para
// que possamos calcular quanto que o mouse se movimentou entre dois instantes
// de tempo. Utilizadas no callback CursorPosCallback() abaixo.
double g_LastCursorPosX, g_LastCursorPosY;

// Função callback chamada sempre que o usuário aperta algum dos botões do mouse
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_LeftMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_LeftMouseButtonPressed = true;
        //createCow(rand()%20 - 10, rand()%20 - 10); // TESTE DAS VACAS

    }

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_LeftMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_RightMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_RightMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_RightMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_MiddleMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_MiddleMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_MiddleMouseButtonPressed = false;
    }
}

// Função callback chamada sempre que o usuário movimentar o cursor do mouse em
// cima da janela OpenGL.
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    // Abaixo executamos o seguinte: caso o botão esquerdo do mouse esteja
    // pressionado, computamos quanto que o mouse se movimento desde o último
    // instante de tempo, e usamos esta movimentação para atualizar os
    // parâmetros que definem a posição da câmera dentro da cena virtual.
    // Assim, temos que o usuário consegue controlar a câmera.

    // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
    float dx = xpos - g_LastCursorPosX;
    float dy = ypos - g_LastCursorPosY;

    // Para a implementação da câmera livre considerou-se que o vetor
    // view será modificado de acordo com normalização da soma do seu
    // valor atual com os vetores de deslocamento em x e em y.
    // Para fazer isso se considera o vetor up da câmera e se faz um
    // produto vetorial entre ele e o view para se obter a direção de
    // um vetor que possa representar o deslocamento horizontal.
    // Em seguida usa-se esse vetor em um produto vetorial com view para
    // se obter o vetor com a mesma direção do vetor que representará o
    // deslocamento vertical

    glm::vec4 HorizCrossProduct = crossproduct(Camera.camera_view, Camera.camera_up);
    //hVec /= norm(hVec);
    glm::vec4 hVec = HorizCrossProduct * (dx * cameraRotationSpeed);

    glm::vec4 vVec = -crossproduct(HorizCrossProduct, Camera.camera_view);;


    vVec /= norm(vVec);
    vVec *= (dy * cameraRotationSpeed);

    glm::vec4 newView = (Camera.camera_view + vVec + hVec)/norm(Camera.camera_view + vVec + hVec);
    //evita que o vetor view fique na mesma direção do vetor up

    glm::vec4 oldViewVector = Camera.camera_view;
    float y = fabs(newView.y);

    float maxy = 0.9f;
    Camera.camera_view += (hVec);
    Camera.camera_view /= norm(Camera.camera_view);

    if(y<= maxy)
    {

        Camera.camera_view += (vVec);
        Camera.camera_view /= norm(Camera.camera_view);

    }
    glm::vec4 newViewVector = Camera.camera_view;
    //projeta no plano xz (horizontal)
    float tempNewY = newViewVector.y;
    float tempOldY = oldViewVector.y;
    oldViewVector.y = 0.0f;
    newViewVector.y = 0.0f;
    float angleWeapon = dotproduct(oldViewVector, newViewVector)/(length(newViewVector) * length(oldViewVector));
    if(angleWeapon>=1 || angleWeapon<=-1)  //necessário para acos() não retornar nan
        angleWeapon = 0;
    else
        angleWeapon = acos(angleWeapon);
    if(dx<0)
    {
        yRotation = yRotation + angleWeapon;
    }
    else
        yRotation = yRotation - angleWeapon;

    oldViewVector.y = tempOldY;
    newViewVector.y = tempNewY;
    //projeta no plano yz (vertical)
    tempNewY = newViewVector.x;
    tempOldY = oldViewVector.x;
    oldViewVector.x = 0.0f;
    newViewVector.x = 0.0f;
    angleWeapon = dotproduct(baseWeaponVector, newViewVector)/(length(newViewVector) * length(baseWeaponVector));
    if(angleWeapon>=1 || angleWeapon<=-1) //necessário para acos() não retornar nan
        angleWeapon = 0;
    else
        angleWeapon = acos(angleWeapon);

    float minX = -1.12;
    float maxX = 1.12;
    if(newViewVector.y<0)
    {
        xRotation = angleWeapon;
    }
    else
        xRotation = -angleWeapon;

    if(xRotation > M_PI/2){
        xRotation = M_PI - xRotation;
    }
    else if(xRotation < - M_PI/2){
        xRotation = -M_PI - xRotation;
    }

    if(xRotation > maxX){
        xRotation = maxX;
    }
    else if(xRotation < minX){
        xRotation = minX;
    }
    oldViewVector.x = tempOldY;
    newViewVector.x = tempNewY;


    // Em coordenadas esféricas, o ângulo phi deve ficar entre -pi/2 e +pi/2.

    /*
    phimax = 3.141592f/2;
    phimin = -phimax;

    if (g_CameraPhi > phimax)
        g_CameraPhi = phimax;

    if (g_CameraPhi < phimin)
        g_CameraPhi = phimin;
*/
    // Atualizamos as variáveis globais para armazenar a posição atual do
    // cursor como sendo a última posição conhecida do cursor.
    g_LastCursorPosX = xpos;
    g_LastCursorPosY = ypos;

}

// Função callback chamada sempre que o usuário movimenta a "rodinha" do mouse.
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // DO NOTHING
}

// Definição da função que será chamada sempre que o usuário pressionar alguma
// tecla do teclado. Veja http://www.glfw.org/docs/latest/input_guide.html#input_key
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    // Se o usuário pressionar a tecla ESC, fechamos a janela.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    //Mover para frente
    if(key == GLFW_KEY_W)
    {
        if(action == GLFW_PRESS){
            if(!isMoving){
                isMoving = true;
            }
            isWPressed = true;
            moveForward();
        }
        if(action == GLFW_RELEASE)
        {
            isWPressed = false;
            if(!(isAPressed || isDPressed || isSPressed))
                isMoving = false;
        }
    }
    //Mover para a esquerda
    if(key == GLFW_KEY_A)
    {

        if(action == GLFW_PRESS){
            if(!isMoving){
                isMoving = true;
            }
            isAPressed = true;
            moveLeft();
        }
        if(action == GLFW_RELEASE)
        {
            isAPressed = false;
            if(!(isDPressed || isWPressed || isSPressed))
                isMoving = false;
        }

    }
    //Mover para trás
    if(key == GLFW_KEY_S)
    {

        if(action == GLFW_PRESS){
            if(!isMoving){
                isMoving = true;
            }
            isSPressed = true;
            moveBack();
        }
        if(action == GLFW_RELEASE)
        {
            isSPressed = false;
            if(!(isAPressed || isWPressed || isDPressed))
                isMoving = false;
        }

    }
    //Mover para a direita
    if(key == GLFW_KEY_D)
    {
        if(action == GLFW_PRESS){
            if(!isMoving){
                isMoving = true;
            }
            isDPressed = true;
            moveRight();
        }
        if(action == GLFW_RELEASE)
        {
            isDPressed = false;
            if(!(isAPressed || isWPressed || isSPressed))
                isMoving = false;
        }
    }

    /**H e R são úteis para depuração, serão controlados pela variável debug*/

    if (debug){
        // Se o usuário apertar a tecla H, fazemos um "toggle" do texto informativo mostrado na tela.
        if (key == GLFW_KEY_H && action == GLFW_PRESS)
        {
            g_ShowInfoText = !g_ShowInfoText;
        }

        // Se o usuário apertar a tecla R, recarregamos os shaders dos arquivos "shader_fragment.glsl" e "shader_vertex.glsl".
        if (key == GLFW_KEY_R && action == GLFW_PRESS)
        {
            LoadShadersFromFiles();
            fprintf(stdout,"Shaders recarregados!\n");
            fflush(stdout);
        }
    }
}

// Definimos o callback para impressão de erros da GLFW no terminal
void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

// Esta função recebe um vértice com coordenadas de modelo p_model e passa o
// mesmo por todos os sistemas de coordenadas armazenados nas matrizes model,
// view, e projection; e escreve na tela as matrizes e pontos resultantes
// dessas transformações.
void TextRendering_ShowModelViewProjection(
    GLFWwindow* window,
    glm::mat4 projection,
    glm::mat4 view,
    glm::mat4 model,
    glm::vec4 p_model
)
{
    if ( !g_ShowInfoText )
        return;

    glm::vec4 p_world = model*p_model;
    glm::vec4 p_camera = view*p_world;

    float pad = TextRendering_LineHeight(window);

    TextRendering_PrintString(window, " Model matrix             Model     World", -1.0f, 1.0f-pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, model, p_model, -1.0f, 1.0f-2*pad, 1.0f);

    TextRendering_PrintString(window, " View matrix              World     Camera", -1.0f, 1.0f-7*pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, view, p_world, -1.0f, 1.0f-8*pad, 1.0f);

    TextRendering_PrintString(window, " Projection matrix        Camera                   NDC", -1.0f, 1.0f-13*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductDivW(window, projection, p_camera, -1.0f, 1.0f-14*pad, 1.0f);
}

// Escrevemos na tela os ângulos de Euler definidos nas variáveis globais
// g_AngleX, g_AngleY, e g_AngleZ.
void TextRendering_ShowEulerAngles(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float pad = TextRendering_LineHeight(window);

    char buffer[80];
    snprintf(buffer, 80, "Euler Angles rotation matrix = Z(%.2f)*Y(%.2f)*X(%.2f)\n", g_AngleZ, g_AngleY, g_AngleX);

    TextRendering_PrintString(window, buffer, -1.0f+pad/10, -1.0f+2*pad/10, 1.0f);
}

// Escrevemos na tela qual matriz de projeção está sendo utilizada.
void TextRendering_ShowProjection(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    if ( g_UsePerspectiveProjection )
        TextRendering_PrintString(window, "Perspective", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
    else
        TextRendering_PrintString(window, "Orthographic", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
}

// Escrevemos na tela o número de quadros renderizados por segundo (frames per
// second).
void TextRendering_ShowFramesPerSecond(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    // Variáveis estáticas (static) mantém seus valores entre chamadas
    // subsequentes da função!
    static float old_seconds = (float)glfwGetTime();
    static int   ellapsed_frames = 0;
    static char  buffer[20] = "?? fps";
    static int   numchars = 7;

    ellapsed_frames += 1;

    // Recuperamos o número de segundos que passou desde a execução do programa
    float seconds = (float)glfwGetTime();

    // Número de segundos desde o último cálculo do fps
    float ellapsed_seconds = seconds - old_seconds;

    if ( ellapsed_seconds > 1.0f )
    {
        numchars = snprintf(buffer, 20, "%.2f fps", ellapsed_frames / ellapsed_seconds);

        old_seconds = seconds;
        ellapsed_frames = 0;
    }

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, 1.0f-(numchars + 1)*charwidth, 1.0f-lineheight, 1.0f);
}

// Função para debugging: imprime no terminal todas informações de um modelo
// geométrico carregado de um arquivo ".obj".
// Veja: https://github.com/syoyo/tinyobjloader/blob/22883def8db9ef1f3ffb9b404318e7dd25fdbb51/loader_example.cc#L98
void PrintObjModelInfo(ObjModel* model)
{
  const tinyobj::attrib_t                & attrib    = model->attrib;
  const std::vector<tinyobj::shape_t>    & shapes    = model->shapes;
  const std::vector<tinyobj::material_t> & materials = model->materials;

  printf("# of vertices  : %d\n", (int)(attrib.vertices.size() / 3));
  printf("# of normals   : %d\n", (int)(attrib.normals.size() / 3));
  printf("# of texcoords : %d\n", (int)(attrib.texcoords.size() / 2));
  printf("# of shapes    : %d\n", (int)shapes.size());
  printf("# of materials : %d\n", (int)materials.size());

  for (size_t v = 0; v < attrib.vertices.size() / 3; v++) {
    printf("  v[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.vertices[3 * v + 0]),
           static_cast<const double>(attrib.vertices[3 * v + 1]),
           static_cast<const double>(attrib.vertices[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.normals.size() / 3; v++) {
    printf("  n[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.normals[3 * v + 0]),
           static_cast<const double>(attrib.normals[3 * v + 1]),
           static_cast<const double>(attrib.normals[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.texcoords.size() / 2; v++) {
    printf("  uv[%ld] = (%f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.texcoords[2 * v + 0]),
           static_cast<const double>(attrib.texcoords[2 * v + 1]));
  }

  // For each shape
  for (size_t i = 0; i < shapes.size(); i++) {
    printf("shape[%ld].name = %s\n", static_cast<long>(i),
           shapes[i].name.c_str());
    printf("Size of shape[%ld].indices: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.indices.size()));

    size_t index_offset = 0;

    assert(shapes[i].mesh.num_face_vertices.size() ==
           shapes[i].mesh.material_ids.size());

    printf("shape[%ld].num_faces: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.num_face_vertices.size()));

    // For each face
    for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++) {
      size_t fnum = shapes[i].mesh.num_face_vertices[f];

      printf("  face[%ld].fnum = %ld\n", static_cast<long>(f),
             static_cast<unsigned long>(fnum));

      // For each vertex in the face
      for (size_t v = 0; v < fnum; v++) {
        tinyobj::index_t idx = shapes[i].mesh.indices[index_offset + v];
        printf("    face[%ld].v[%ld].idx = %d/%d/%d\n", static_cast<long>(f),
               static_cast<long>(v), idx.vertex_index, idx.normal_index,
               idx.texcoord_index);
      }

      printf("  face[%ld].material_id = %d\n", static_cast<long>(f),
             shapes[i].mesh.material_ids[f]);

      index_offset += fnum;
    }

    printf("shape[%ld].num_tags: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.tags.size()));
    for (size_t t = 0; t < shapes[i].mesh.tags.size(); t++) {
      printf("  tag[%ld] = %s ", static_cast<long>(t),
             shapes[i].mesh.tags[t].name.c_str());
      printf(" ints: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].intValues.size(); ++j) {
        printf("%ld", static_cast<long>(shapes[i].mesh.tags[t].intValues[j]));
        if (j < (shapes[i].mesh.tags[t].intValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" floats: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].floatValues.size(); ++j) {
        printf("%f", static_cast<const double>(
                         shapes[i].mesh.tags[t].floatValues[j]));
        if (j < (shapes[i].mesh.tags[t].floatValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" strings: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].stringValues.size(); ++j) {
        printf("%s", shapes[i].mesh.tags[t].stringValues[j].c_str());
        if (j < (shapes[i].mesh.tags[t].stringValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");
      printf("\n");
    }
  }

  for (size_t i = 0; i < materials.size(); i++) {
    printf("material[%ld].name = %s\n", static_cast<long>(i),
           materials[i].name.c_str());
    printf("  material.Ka = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].ambient[0]),
           static_cast<const double>(materials[i].ambient[1]),
           static_cast<const double>(materials[i].ambient[2]));
    printf("  material.Kd = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].diffuse[0]),
           static_cast<const double>(materials[i].diffuse[1]),
           static_cast<const double>(materials[i].diffuse[2]));
    printf("  material.Ks = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].specular[0]),
           static_cast<const double>(materials[i].specular[1]),
           static_cast<const double>(materials[i].specular[2]));
    printf("  material.Tr = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].transmittance[0]),
           static_cast<const double>(materials[i].transmittance[1]),
           static_cast<const double>(materials[i].transmittance[2]));
    printf("  material.Ke = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].emission[0]),
           static_cast<const double>(materials[i].emission[1]),
           static_cast<const double>(materials[i].emission[2]));
    printf("  material.Ns = %f\n",
           static_cast<const double>(materials[i].shininess));
    printf("  material.Ni = %f\n", static_cast<const double>(materials[i].ior));
    printf("  material.dissolve = %f\n",
           static_cast<const double>(materials[i].dissolve));
    printf("  material.illum = %d\n", materials[i].illum);
    printf("  material.map_Ka = %s\n", materials[i].ambient_texname.c_str());
    printf("  material.map_Kd = %s\n", materials[i].diffuse_texname.c_str());
    printf("  material.map_Ks = %s\n", materials[i].specular_texname.c_str());
    printf("  material.map_Ns = %s\n",
           materials[i].specular_highlight_texname.c_str());
    printf("  material.map_bump = %s\n", materials[i].bump_texname.c_str());
    printf("  material.map_d = %s\n", materials[i].alpha_texname.c_str());
    printf("  material.disp = %s\n", materials[i].displacement_texname.c_str());
    printf("  <<PBR>>\n");
    printf("  material.Pr     = %f\n", materials[i].roughness);
    printf("  material.Pm     = %f\n", materials[i].metallic);
    printf("  material.Ps     = %f\n", materials[i].sheen);
    printf("  material.Pc     = %f\n", materials[i].clearcoat_thickness);
    printf("  material.Pcr    = %f\n", materials[i].clearcoat_thickness);
    printf("  material.aniso  = %f\n", materials[i].anisotropy);
    printf("  material.anisor = %f\n", materials[i].anisotropy_rotation);
    printf("  material.map_Ke = %s\n", materials[i].emissive_texname.c_str());
    printf("  material.map_Pr = %s\n", materials[i].roughness_texname.c_str());
    printf("  material.map_Pm = %s\n", materials[i].metallic_texname.c_str());
    printf("  material.map_Ps = %s\n", materials[i].sheen_texname.c_str());
    printf("  material.norm   = %s\n", materials[i].normal_texname.c_str());
    std::map<std::string, std::string>::const_iterator it(
        materials[i].unknown_parameter.begin());
    std::map<std::string, std::string>::const_iterator itEnd(
        materials[i].unknown_parameter.end());

    for (; it != itEnd; it++) {
      printf("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
    }
    printf("\n");
  }
}

void setCameraPosition(float x, float y, float z)
{
    Camera.camera_position.x = x;
    Camera.camera_position.y = y;
    Camera.camera_position.z = z;

}

/*void setWeaponPosition(float deltax, float deltay, float deltaz)
{
    model = model *  Matrix_Translate(deltax, deltay, deltaz);

}*/

/*void setWeaponAngle(float anglex, float angley, float anglez)
{
    model = model
                      * Matrix_Rotate_Z(anglez)
                      * Matrix_Rotate_Y(angley)
                      * Matrix_Rotate_X(anglex);
}*/

/*void setWeaponAngle2(float angle, glm::vec4 view)
{
    model = model * Matrix_Rotate(angle, view);
}*/

glm::vec3 toSpherical(float x, float y, float z)
{
    float d = sqrt(x*x + y*y + z*z);
    float t = atan2(y,x);
    float p = atan2(d*sin(t),z);
    glm::vec3 v = glm::vec3(d, t, p);
    return v;
}

void moveForward()
{

    float x = Camera.camera_position.x;
    float y = Camera.camera_position.y;
    float z = Camera.camera_position.z;

    float viewVectorNorm = norm(Camera.camera_view);
    float nx = x + movementSpeed * ellapsed_time * Camera.camera_view.x/viewVectorNorm;
    float ny = y; // não altera a altura da câmera
    float nz = z + movementSpeed * ellapsed_time * Camera.camera_view.z/viewVectorNorm;

    //cada repeticao do mapa tem tamanho 1, se subtrai 0.5 para não ficar com impressao de que metade do jogador está para fora
    if(fabs(nx) >= MAP_X_REPEAT - 0.5){
        nx = x;
    }
    if(fabs(nz) >= MAP_Z_REPEAT - 0.5){
        nz = z;
    }
    setCameraPosition(nx, ny, nz);

}
//função muito semelhante a função moveForward, separada para melhor legibilidade
//no chamamento da função
void moveBack()
{
    float x = Camera.camera_position.x;
    float y = Camera.camera_position.y;
    float z = Camera.camera_position.z;


    float viewVectorNorm = norm(Camera.camera_view);
    float nx = x - movementSpeed * ellapsed_time * Camera.camera_view.x/viewVectorNorm;
    float ny = y; // não altera a altura da câmera
    float nz = z - movementSpeed * ellapsed_time * Camera.camera_view.z/viewVectorNorm;
    //cada repeticao do mapa tem tamanho 1, se subtrai 0.5 para não ficar com impressao de que metade do jogador está para fora
    if(fabs(nx) >= MAP_X_REPEAT - 0.5){
        nx = x;
    }
    if(fabs(nz) >= MAP_Z_REPEAT - 0.5){
        nz = z;
    }
    setCameraPosition(nx, ny, nz);
}
void moveLeft()
{
    float x = Camera.camera_position.x;
    float y = Camera.camera_position.y;
    float z = Camera.camera_position.z;

    // Abaixo definimos as varáveis que efetivamente definem a câmera virtual.
    // Veja slides 165-175 do documento "Aula_08_Sistemas_de_Coordenadas.pdf".

    glm::vec4 camera_u = -crossproduct(Camera.camera_up, -(Camera.camera_view)/*w*/);
    float uVectorNorm = norm(camera_u);

    float nx = x + ellapsed_time * movementSpeed * camera_u.x/uVectorNorm;
    float ny = y; // não altera a altura da câmera
    float nz = z + ellapsed_time * movementSpeed * camera_u.z/uVectorNorm;
    //cada repeticao do mapa tem tamanho 1, se subtrai 0.5 para não ficar com impressao de que metade do jogador está para fora
    if(fabs(nx) >= MAP_X_REPEAT - 0.5){
        nx = x;
    }
    if(fabs(nz) >= MAP_Z_REPEAT - 0.5){
        nz = z;
    }
    setCameraPosition(nx, ny, nz);
}
void moveRight()
{

    float x = Camera.camera_position.x;
    float y = Camera.camera_position.y;
    float z = Camera.camera_position.z;

    // Abaixo definimos as varáveis que efetivamente definem a câmera virtual.
    // Veja slides 165-175 do documento "Aula_08_Sistemas_de_Coordenadas.pdf".

    glm::vec4 camera_u = crossproduct(Camera.camera_up, -(Camera.camera_view)/*w*/);
    float uVectorNorm = norm(camera_u);

    float nx = x + movementSpeed * camera_u.x/uVectorNorm * ellapsed_time;
    float ny = y; // não altera a altura da câmera
    float nz = z + movementSpeed * camera_u.z/uVectorNorm * ellapsed_time;
    //cada repeticao do mapa tem tamanho 1, se subtrai 0.5 para não ficar com impressao de que metade do jogador está para fora
    if(fabs(nx) >= MAP_X_REPEAT - 0.5){
        nx = x;
    }
    if(fabs(nz) >= MAP_Z_REPEAT - 0.5){
        nz = z;
    }
    setCameraPosition(nx, ny, nz);
}

//somente deve ser chamada quando verificou-se que isMoving = true
void handleMovement()
{
    if(isWPressed)
        moveForward();
    if(isSPressed)
        moveBack();
    if(isAPressed)
       moveLeft();
    if(isDPressed)
        moveRight();

}


int createCow(double xpos, double zpos)
{
    Cow* calf = (Cow*) malloc(sizeof(Cow));
    calf->id = currentCowId;
    calf->xpos = xpos;
    calf->zpos = zpos;
    calf->angle = 0;
    calf->angularMovementDirection = UP;

    currentCowId++;

    CowList* newNode = (CowList*) malloc(sizeof(CowList));
    newNode->currentCow = calf;
    newNode->next = NULL;

    CowList* tempCows = cows;
    CowList* prevCows = NULL;
    if(tempCows == NULL){
        cows = newNode;
    }
    else{
        while(tempCows!= NULL)
        {
            prevCows = tempCows;
            tempCows = tempCows->next;
        }
        prevCows->next = newNode;
    }
    currentLivingCows++;

    return calf->id;
}
void removeCow(int id)
{
    CowList* tempCows = cows;
    CowList* prevCows = NULL;

    bool found = false;
    while(tempCows!= NULL && !found){
        if(tempCows->currentCow->id == id){
            found = true;

        }
        else{
            prevCows = tempCows;
            tempCows = tempCows->next;
        }
    }
    if(tempCows == NULL)
    {
        //não encontrou o id, logo faz nada

    }
    else if(found){
        if(prevCows == NULL) // primeiro da lista
        {
            cows = tempCows->next;
        }
        else{
            prevCows->next = tempCows->next;
            free(tempCows);

        }
        currentLivingCows--;
    }
}

void updateCows()
{
    CowList* tempCows = cows;
    while(tempCows!= NULL)
    {



        Cow* currentCow = tempCows->currentCow;
        glm::vec2 desloc = glm::vec2(Camera.camera_position.x - currentCow->xpos, Camera.camera_position.z - currentCow->zpos);
        desloc = desloc / length(desloc);

        currentCow->xpos += (desloc.x * cowSpeed * ellapsed_time);
        currentCow->zpos += (desloc.y * cowSpeed * ellapsed_time);
        double distanceToCamera = sqrt(pow(currentCow->xpos - Camera.camera_position.x, 2) + pow(currentCow->zpos - Camera.camera_position.z, 2));
        if (distanceToCamera < minDistanceToCow){
            removeCow(currentCow->id);
        }
        double angle;
        if (currentCow->angularMovementDirection == UP){
            angle = currentCow->angle + cowAngularSpeed * ellapsed_time;
            if(angle > MAX_ANGLE){
                currentCow->angularMovementDirection = DOWN;
            }



        }
        else{
            angle = currentCow->angle - cowAngularSpeed * ellapsed_time;
            if(angle < MIN_ANGLE){
                currentCow->angularMovementDirection = UP;
            }
        }


        //as duas condições abaixo servem para evitar que o angulo se altere muito quando a
        //renderização está pausada por movimento da janela
        if(angle > 1.2 * MAX_ANGLE){
            angle = MAX_ANGLE;
            currentCow->angularMovementDirection = DOWN;
        }
        if(angle < 1.2 * MIN_ANGLE){
            angle = MIN_ANGLE;
            currentCow->angularMovementDirection = UP;
        }
        currentCow->angle = angle;
        tempCows = tempCows->next;
    }
}

void drawCows()
{
    CowList* tempCows = cows;
    while(tempCows!= NULL)
    {
        double angleToCamera = cowAngleToCamera(tempCows->currentCow->xpos, tempCows->currentCow->zpos);
        glm::mat4 model = Matrix_Translate(tempCows->currentCow->xpos, 0.0f, tempCows->currentCow->zpos)
                        * Matrix_Identity()
                        * Matrix_Rotate_Y(angleToCamera)
                        * Matrix_Rotate_Z(tempCows->currentCow->angle);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(object_id_uniform, COW);
        DrawVirtualObject("cow");
        tempCows = tempCows->next;
    }
}

//determina o quanto a vaca deve rotacionar para olhar para o jogador
double cowAngleToCamera(double xpos, double zpos)
{
    glm::vec4 baseCowView (1.0, 0.0, 0.0, 0.0);
    glm::vec4 cowToPlayerVec (Camera.camera_position.x - xpos, 0.0, Camera.camera_position.z - zpos, 0.0);
    double angle = acos(dotproduct(baseCowView, cowToPlayerVec) / ( length(baseCowView) * length(cowToPlayerVec) ));
    if(Camera.camera_position.z > zpos)
    {
        angle = M_PI * 2 - angle;
    }
    return angle;
}

int createBullet()
{
    Bullet* newBullet = (Bullet*) malloc(sizeof(Bullet));
    newBullet->id = currentBulletId;
    newBullet->pos = Camera.camera_position;
    newBullet->direction = Camera.camera_view;
    currentBulletId++;

    BulletList* newNode = (BulletList*) malloc(sizeof(BulletList));
    newNode->currentBullet = newBullet;
    newNode->next = NULL;

    BulletList* tempBullet = bullets;
    BulletList* prevBullet = NULL;
    if(tempBullet == NULL){
        bullets = newNode;
    }
    else{
        while(tempBullet!= NULL)
        {
            prevBullet = tempBullet;
            tempBullet = tempBullet->next;
        }
        prevBullet->next = newNode;
    }

    return newBullet->id;
}
void removeBullet(int id)
{
    BulletList* tempBullet = bullets;
    BulletList* prevBullet = NULL;

    bool found = false;
    while(tempBullet!= NULL && !found){
        if(tempBullet->currentBullet->id == id){
            found = true;

        }
        else{
            prevBullet = tempBullet;
            tempBullet = tempBullet->next;
        }
    }
    if(tempBullet == NULL)
    {
        //não encontrou o id, logo faz nada

    }
    else if(found){
        if(prevBullet == NULL) // primeiro da lista
        {
            bullets = tempBullet->next;
        }
        else{
            prevBullet->next = tempBullet->next;
            free(tempBullet);
        }
    }
}
void updateBullet()
{
    BulletList* tempBullet = bullets;
    while(tempBullet!= NULL)
    {
        Bullet* currentBullet = tempBullet->currentBullet;
        glm::vec4 prevPos = currentBullet->pos;
        glm::vec4 deslocVec = glm::vec4(currentBullet->direction.x * BULLET_SPEED * ellapsed_time, currentBullet->direction.y * BULLET_SPEED * ellapsed_time, currentBullet->direction.z * BULLET_SPEED * ellapsed_time, 0.0f);
        currentBullet->pos += deslocVec;
        checkCollisionWithCows(prevPos, currentBullet->pos);
        if(norm(currentBullet->pos) > MAX_DISTANCE_TO_BULLET)
        {
            removeBullet(currentBullet->id);
        }
        tempBullet = tempBullet->next;
    }
}

void drawBullet()
{
    BulletList* tempBullets = bullets;
    while (tempBullets!= NULL)
    {
        Bullet* bullet = tempBullets->currentBullet;

        glm::mat4 model = Matrix_Translate(bullet->pos.x, bullet->pos.y, bullet->pos.z)
                        * Matrix_Scale(0.1, 0.1, 0.1)
                        * Matrix_Identity();
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(object_id_uniform, BULLET);
        DrawVirtualObject("bullet");
        tempBullets = tempBullets->next;

    }
}

//checa se o segmento de reta entre a posição antiga e a nova da bala colidem com alguma vaca, se sim
//remove ambos
void checkCollisionWithCows(glm::vec4 prevPos, glm::vec4 currentPos){
    CowList* tempCows = cows;
    while(tempCows!=NULL)
    {
        BulletList* tempBullets = bullets;
        while(tempBullets!=NULL){
            if (boundingBoxCollided(tempCows->currentCow, tempBullets->currentBullet->pos)){
                removeCow(tempCows->currentCow->id);
                removeBullet(tempBullets->currentBullet->id);
            }
            tempBullets = tempBullets->next;
        }
        tempCows = tempCows->next;
    }
}

void updateRecoil()
{
    if (recoilDirection == UP){
        currentRecoilAngle+=recoilSpeed*ellapsed_time;
        if(currentRecoilAngle > GUN_RECOIL_ANGLE)
            recoilDirection = DOWN;
    }
    else if (recoilDirection == DOWN)
    {
        currentRecoilAngle-=recoilSpeed*ellapsed_time;
        if(currentRecoilAngle < 0){
            recoilDirection = UP;
            onRecoil = false;
            currentRecoilAngle = 0.0f;
        }
    }

}
void shoot()
{
    createBullet();
    onRecoil = true;
    currentShotDelay = shotDelay;

}

//verifica se um ponto esta dentro da bounding box da vaca, tem menor custo computacional que uma verificação mais fina

bool boundingBoxCollided(Cow* cow, glm::vec4 pos)
{

    glm::vec4 upRightBackPnt = glm::vec4(cow->xpos + 0.5, 0.8, cow->zpos + 0.5, 1.0);
    glm::vec4 downLeftFrontPnt = glm::vec4(cow->xpos -0.5, -0.8, cow->zpos -0.5, 1.0);

    return(pos.x >= downLeftFrontPnt.x && pos.x <= upRightBackPnt.x &&
       pos.y >= downLeftFrontPnt.y && pos.y <= upRightBackPnt.y &&
       pos.z >= downLeftFrontPnt.z && pos.z <= upRightBackPnt.z);
}



// set makeprg=cd\ ..\ &&\ make\ run\ >/dev/null
// vim: set spell spelllang=pt_br :
// set makeprg=cd\ ..\ &&\ make\ run\ >/dev/null
// vim: set spell spelllang=pt_br :
