#version 330 core

// Atributos de fragmentos recebidos como entrada ("in") pelo Fragment Shader.
// Neste exemplo, este atributo foi gerado pelo rasterizador como a
// interpolação da cor de cada vértice, definidas em "shader_vertex.glsl" e
// "main.cpp".
in vec4 position_world;
in vec4 normal;

// Posição do vértice atual no sistema de coordenadas local do modelo.
in vec4 position_model;

// Coordenadas de textura obtidas do arquivo OBJ (se existirem!)
in vec2 texcoords;

// Matrizes computadas no código C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Identificador que define qual objeto está sendo desenhado no momento
#define SPHERE 0
#define COW  1
#define PLANE  2
#define M4A1 3
#define CHAIR 4
#define BULLET 5
#define WORLDSPHERE 6
uniform int object_id;

// Parâmetros da axis-aligned bounding box (AABB) do modelo
uniform vec4 bbox_min;
uniform vec4 bbox_max;

// Variáveis para acesso das imagens de textura
uniform sampler2D TextureImage0;
uniform sampler2D TextureImage1;
uniform sampler2D TextureImage2;
uniform sampler2D TextureImage3;
uniform sampler2D TextureImage4;
// O valor de saída ("out") de um Fragment Shader é a cor final do fragmento.
out vec3 color;

// Constantes
#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923

void main()
{
    // Obtemos a posição da câmera utilizando a inversa da matriz que define o
    // sistema de coordenadas da câmera.
    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 camera_position = inverse(view) * origin;

    // O fragmento atual é coberto por um ponto que percente à superfície de um
    // dos objetos virtuais da cena. Este ponto, p, possui uma posição no
    // sistema de coordenadas global (World coordinates). Esta posição é obtida
    // através da interpolação, feita pelo rasterizador, da posição de cada
    // vértice.
    vec4 p = position_world;

    // Normal do fragmento atual, interpolada pelo rasterizador a partir das
    // normais de cada vértice.
    vec4 n = normalize(normal);

    // Vetor que define o sentido da fonte de luz em relação ao ponto atual.
    vec4 l = normalize(vec4(1.0,1.0,0.0,0.0));

    // Vetor que define o sentido da câmera em relação ao ponto atual.
    vec4 v = normalize(camera_position - p);

    vec4 r = -l + 2*n*dot(n,l);

    vec4 halfVector = (v + l) / length(v+l);
    vec3 Kd; // refletência difusa
    vec3 Ks; // Refletância especular
    vec3 Ka; // refletância ambiente

    float q;

    // Coordenadas de textura U e V
    float U = 0.0;
    float V = 0.0;

    bool fixColorObject = false;
    bool lambertShading = false;
    if ( object_id == COW )
    {
        // PREENCHA AQUI as coordenadas de textura do coelho, computadas com
        // projeção planar XY em COORDENADAS DO MODELO. Utilize como referência
        // o slide 106 do documento "Aula_20_e_21_Mapeamento_de_Texturas.pdf",
        // e também use as variáveis min*/max* definidas abaixo para normalizar
        // as coordenadas de textura U e V dentro do intervalo [0,1]. Para
        // tanto, veja por exemplo o mapeamento da variável 'p_v' utilizando
        // 'h' no slide 151 do documento "Aula_20_e_21_Mapeamento_de_Texturas.pdf".

        float minx = bbox_min.x;
        float maxx = bbox_max.x;

        float miny = bbox_min.y;
        float maxy = bbox_max.y;

        float minz = bbox_min.z;
        float maxz = bbox_max.z;

        U = (position_model.x -minx) / (maxx - minx);
        V = (position_model.y -miny) / (maxy - miny);
        // Obtemos a refletância difusa a partir da leitura da imagem TextureImage0
        Kd = texture(TextureImage0, vec2(U,V)).rgb;
        Ka = vec3(0.02, 0.02, 0.02);
        Ks = vec3(0.01, 0.01, 0.01);
        q = 8.0;
    }
    else if(object_id == PLANE)
    {
        int xRepeat = int(floor(texcoords.x*20.0));
        int yRepeat = int(floor(texcoords.y*20.0));

        U = (texcoords.x)*20 - xRepeat;
        V = (texcoords.y)*20 - yRepeat;

        // Obtemos a refletância difusa a partir da leitura da imagem TextureImage0
        Kd = texture(TextureImage1, vec2(U,V)).rgb;
        /*Ka = vec3(0.00, 0.00, 0.00);
        Ks = vec3(0.0, 0.0, 0.0);*/
        Ka = vec3(0.02, 0.02, 0.02);
        Ks = vec3(0.01, 0.01, 0.01);
        q = 8.0;
    }
    else if ( object_id == M4A1 )
    {
        float minx = bbox_min.x;
        float maxx = bbox_max.x;

        float miny = bbox_min.y;
        float maxy = bbox_max.y;

        float minz = bbox_min.z;
        float maxz = bbox_max.z;

        U = (position_model.x -minx) / (maxx - minx);
        V = (position_model.y -miny) / (maxy - miny);

        Kd = texture(TextureImage2, vec2(U,V)).rgb;
        Ka = vec3(0.02, 0.02, 0.02);
        Ks = vec3(0.6, 0.6, 0.6);

        q = 64.0;

    }
    else if ( object_id == CHAIR )
    {
        Kd = vec3(1.0, 1.0, 0.0);
        Ka = vec3(0.0, 0.0, 0.0);
        Ks = vec3(0.0, 0.0, 0.0);
        q = 0.0;
        fixColorObject = true;
    }
    else if(object_id == BULLET){
        Kd = vec3(0.7, 0.4, 0.1);
        Ka = vec3(0.2, 0.2, 0.2);
        lambertShading = true;
    }
    else if(object_id == WORLDSPHERE){
        vec4 bbox_center = (bbox_min + bbox_max) / 2.0;

        float sphere_radius = sqrt(pow(position_model[0], 2) + pow(position_model[1], 2) + pow(position_model[2], 2));
        vec4 p_vector = (position_model - bbox_center)/length(position_model - bbox_center);
        vec4 sphere_p = bbox_center + sphere_radius*p_vector;
        float theta = atan(sphere_p[0], sphere_p[2]);
        float phi = asin(sphere_p[1]/sphere_radius);

        U = (theta + M_PI)/(2*M_PI);
        V = (phi + M_PI_2)/M_PI;

        Kd = texture(TextureImage4, vec2(U,V)).rgb;
        Ka = vec3(0.0, 0.0, 0.0);
        Ks = vec3(0.0, 0.0, 0.0);
        q = 0.0;

        fixColorObject = true;
    }
    else{ // objeto desconhecido
        Kd = vec3(0.0, 0.0, 0.0);
        Ks = vec3(0.0, 0.0, 0.0);
        Ka = vec3(0.0, 0.0, 0.0);
        q = 1.0;
    }

    if (fixColorObject){
        color = Kd;
        color = pow(color, vec3(1.0, 1.0, 1.0)/2.2);
    }
    else if(lambertShading){
        vec3 I = vec3(1.0, 1.0, 1.0);
        vec3 Ia = vec3(0.05, 0.05, 0.05);
        color = Kd*I*max(0.0, dot(n, l))+Ka*Ia;
        color = pow(color, vec3(1.0,1.0,1.0)/2.2);
    }
    else{
        //espectro da fonte de iluminação
        vec3 I = vec3(1.0, 1.0, 1.0); // luz branca
        //espectro da luz ambiente
        vec3 Ia = vec3(0.05, 0.05, 0.05);

        vec3 lambert_diffuse_term = Kd * I * max(0.0, dot(n, l));
        vec3 ambient_term = Ka * Ia;
        vec3 phong_specular_term = Ks * I * pow(max(0, dot(n,halfVector)),q);

        color = lambert_diffuse_term + ambient_term + phong_specular_term;
        color = pow(color, vec3(1.0,1.0,1.0)/2.2);
    }

}
