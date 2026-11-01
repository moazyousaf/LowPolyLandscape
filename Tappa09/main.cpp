#include <SFML/Window.hpp>
#include <glad/glad.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cmath>
#include <cstdlib>

const int N = 50; //griglia 50x50
const float SIZE = 10.0f;

//Vertex Shader
const char* vertSrc = R"(
#version 410 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec3 aNormal;
uniform mat4 MVP;
out vec3 fragColor;
out vec3 fragNormal;
out vec3 fragPos;
void main() {
    gl_Position = MVP * vec4(aPos, 1.0);
    fragColor = aColor;
    fragNormal = aNormal;
    fragPos = aPos; //posizione nel mondo
}
)";

//Fragment Shader
const char* fragSrc = R"(
#version 410 core
in vec3 fragColor;
in vec3 fragNormal;
in vec3 fragPos;
uniform vec3 lightDir; //Direzione in cui viaggia la luce
uniform vec3 cameraPos;
uniform vec3 fogColor;
uniform float fogStart;
uniform float fogEnd;
out vec4 outColor;
void main() {
    vec3 n = normalize(fragNormal);

    float ambient = 0.2;
    float diffuse = max(dot(n, -lightDir), 0.0);
    vec3 lit = fragColor * (ambient + diffuse);

    //Nebbia: 0 = nessuna, 1 = solo nebbia
    float dist = length(fragPos - cameraPos);
    float fogFactor = clamp((dist - fogStart) / (fogEnd - fogStart), 0.0, 1.0);

    outColor = vec4(mix(lit, fogColor, fogFactor), 1.0);
}
)";

//Compila un shader e controlla errori
unsigned int compileShader(unsigned int type, const char* src) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        std::cerr << "Errore shader: " << log << std::endl;
    }
    return shader;
}

//Crea il programma shader (vertex + fragment)
unsigned int createProgram() {
    unsigned int vert = compileShader(GL_VERTEX_SHADER,   vertSrc);
    unsigned int frag = compileShader(GL_FRAGMENT_SHADER, fragSrc);

    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);

    glDeleteShader(vert);
    glDeleteShader(frag);
    return prog;
}

//Restituisce un colore RGB in base all'altezza h
glm::vec3 altitudeColor(float h){
    if(h < -0.5f) return glm::vec3(0.1f, 0.3f, 0.8f); //Acqua blu
    if(h < 0.5f) return glm::vec3(0.8f, 0.7f, 0.4f); //Sabbia giallo
    if(h < 2.0f) return glm::vec3(0.2f, 0.6f, 0.2f); //Erba verde

    return glm::vec3(0.9f, 0.9f, 0.9f); //Neve bianco
    
}

//Altezza del terreno in un punto (x, z)
float terrainHeight(float x, float z){
    float hills = sin(x * 0.3f) * cos(z * 0.3f) * 3.0f; //Colline
    float bumps = sin(x * 0.9f) * cos(z * 0.8f) * 0.5f; //Irregolarità
    return hills + bumps;
}


//Punto 3D della griglia alla posizione (x, z) in indici interi
glm::vec3 gridPoint(int x, int z){
    float step = 2.0f * SIZE / (N - 1);
    float fx = -SIZE + x * step;
    float fz = -SIZE + z * step;
    return glm::vec3(fx, terrainHeight(fx, fz), fz);
}

//Aggiunge un triangolo al vettore: 3 vertici, ognuno con posizione, colore e normale
void addTriangle(std::vector<float>&v, glm::vec3 a, glm::vec3 b, glm::vec3 c){
    
    //La normale della faccia: prodotto vettoriale di due lati
    glm::vec3 normal = glm::normalize(glm::cross(b-a, c-a));

    //Tutta la faccia con un solo colore preso dall'altezza
    float avgHeight = (a.y + b.y + c.y) / 3.0f;
    glm::vec3 col = altitudeColor(avgHeight);

    glm::vec3 pts[3] = { a, b, c};
    for(int i = 0; i < 3; i++){
        v.push_back(pts[i].x); v.push_back(pts[i].y); v.push_back(pts[i].z);
        v.push_back(col.r); v.push_back(col.g); v.push_back(col.b);
        v.push_back(normal.x); v.push_back(normal.y); v.push_back(normal.z); 
    }
}

//addTriangle con colore arbitrario per l'acqua
void addColoredTriangle(std::vector<float>& v,
                        glm::vec3 a, glm::vec3 b, glm::vec3 c,
                        glm::vec3 col) {
    glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));

    glm::vec3 pts[3] = { a, b, c };
    for (int i = 0; i < 3; i++) {
        v.push_back(pts[i].x); v.push_back(pts[i].y); v.push_back(pts[i].z);
        v.push_back(col.r);    v.push_back(col.g);    v.push_back(col.b);
        v.push_back(normal.x); v.push_back(normal.y); v.push_back(normal.z);
    }
}

const int SEGMENTS = 6;
const float PI = 3.14159265f;

//Cono con la base circolare e la punta
void addCone(std::vector<float>& v, glm::vec3 base,
            float radius, float height, glm::vec3 col){
    glm::vec3 apex = base + glm::vec3(0.0f, height, 0.0f);

    for(int i = 0; i < SEGMENTS; i++){
        float a0 = 2.0f * PI * i / SEGMENTS;
        float a1 = 2.0f * PI * (i + 1) / SEGMENTS;

        glm::vec3 p0 = base + glm::vec3(radius * cos(a0), 0.0f, radius * sin(a0));
        glm::vec3 p1 = base + glm::vec3(radius * cos(a1), 0.0f, radius * sin(a1));
        
        //Un triangolo laterale per ogni segmento
        addColoredTriangle(v, p0, apex, p1, col);
    }
    
}

//Cilindro (tronco)
void addCylinder(std::vector<float>& v, glm::vec3 base,
                float radius, float height, glm::vec3 col){
    glm::vec3 up(0.0f, height, 0.0f);

    for(int i = 0; i < SEGMENTS; i++){
        float a0 = 2.0f * PI * i / SEGMENTS;
        float a1 = 2.0f * PI * (i + 1) / SEGMENTS;

        glm::vec3 p0b = base + glm::vec3(radius * cos(a0), 0.0f, radius * sin(a0));
        glm::vec3 p1b = base + glm::vec3(radius * cos(a1), 0.0f, radius * sin(a1));
        glm::vec3 p0t = p0b + up;
        glm::vec3 p1t = p1b + up;

        //Il lato di ogni segmento è un rettangolo, ovvero 2 triangoli
        addColoredTriangle(v, p0b, p0t, p1b, col);
        addColoredTriangle(v, p1b, p0t, p1t, col);
    }
}

//Albero
void addTree(std::vector<float>& v, glm::vec3 base, float scale){
    glm::vec3 brown(0.4f, 0.25f, 0.1f);
    glm::vec3 green(0.1f, 0.45f, 0.15f);

    addCylinder(v, base, 0.012f * scale, 0.06f * scale, brown);
    addCone(v, base + glm::vec3(0.0f, 0.05f * scale, 0.0f),
            0.05f * scale, 0.15f * scale, green);
}

//Rocce
void addRock(std::vector<float>& v, glm::vec3 pos, glm::vec3 size,
            float angleY, glm::vec3 col){
    glm::mat4 M = glm::translate(glm::mat4(1.0f), pos);
    M = glm::rotate(M, angleY, glm::vec3(0.0f, 1.0f, 0.0f));
    M = glm::rotate(M, 0.3f, glm::vec3(1.0f, 0.0f, 0.0f));
    M = glm::scale(M, size);

    //8 angoli del cubo unitario centrato nell'origine
    glm::vec3 corners[8] = {
        {-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f},
        { 0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f},
        {-0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f,  0.5f},
        { 0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f}
    };

    //Applica la trasformazione ai punti
    glm::vec3 p[8];
    for(int i = 0; i < 8; i++)
        p[i] = glm::vec3(M * glm::vec4(corners[i], 1.0f));

    //Una faccia = 2 triangoli
    auto quad = [&](int a, int b, int c, int d){
        addColoredTriangle(v, p[a], p[b], p[c], col);
        addColoredTriangle(v, p[a], p[c], p[d], col);
    };

    quad(4, 5, 6, 7); //davanti
    quad(1, 0, 3, 2); //dietro
    quad(5, 1, 2, 6); //destra
    quad(0, 4, 7, 3); //sinistra
    quad(3, 7, 6, 2); //sopra
}

//Crea VAO e VBO per un vettore di vertici (9 float: posizione, colore, normale)
void createMesh(const std::vector<float>& v, unsigned int& vao, unsigned int& vbo) {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     v.size() * sizeof(float),
                     v.data(), GL_STATIC_DRAW);

        // Attributo 0: posizione
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                              9 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Attributo 1: colore
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                              9 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // Attributo 2: normale
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE,
                              9 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

//Generatore numeri casuali tra 0 e 1
float randf(){
    return (float)std::rand() / (float)RAND_MAX;
}

int main() {
    //Crea finestra SFML con contesto OpenGL 4.1 Core
    sf::ContextSettings settings;
    settings.depthBits      = 24;
    settings.stencilBits    = 8;
    settings.majorVersion   = 4;
    settings.minorVersion   = 1;
    settings.attributeFlags = sf::ContextSettings::Core;

    sf::Window window(
        sf::VideoMode({800u, 600u}),
        "Low-Poly Landscape - Tappa 9",
        sf::Style::Close | sf::Style::Titlebar,
        sf::State::Windowed,
        settings
    );
    window.setVerticalSyncEnabled(true);

    //Inizializza glad
    if (!gladLoadGL()) {
        std::cerr << "Errore: GLAD non inizializzato" << std::endl;
        return -1;
    }

    //Abilitazione depth test per la pulizia del buffer
    glEnable(GL_DEPTH_TEST);
    std::cout << "OpenGL: " << glGetString(GL_VERSION) << std::endl;

    //Generazione di vertici della griglia (con altezza variabile)
    std::vector<float> vertices;
    for(int z = 0; z < N - 1; z++){
        for(int x = 0; x < N - 1; x++){
            glm::vec3 tl = gridPoint(x, z);
            glm::vec3 tr = gridPoint(x + 1, z);
            glm::vec3 bl = gridPoint(x, z + 1);
            glm::vec3 br = gridPoint(x + 1, z + 1);
            
            addTriangle(vertices, tl, bl, tr); //triangolo 1
            addTriangle(vertices, tr, bl, br); //triangolo 2
            
        }
    }
    int vertexCount = (int)(vertices.size() / 9); //9 float per vertice

    unsigned int VAO, VBO;
    createMesh(vertices, VAO, VBO);

    //Piano d'acqua: un quadrato orizzontale più grande del terreno
    float waterLevel = 0.0f;
    float w = SIZE * 4.0f;
    glm::vec3 waterColor(0.1f, 0.3f, 0.8f);

    glm::vec3 wtl(-w, waterLevel, -w);
    glm::vec3 wtr( w, waterLevel, -w);
    glm::vec3 wbl(-w, waterLevel,  w);
    glm::vec3 wbr( w, waterLevel,  w);

    std::vector<float> waterVertices;
    addColoredTriangle(waterVertices, wtl, wbl, wtr, waterColor);
    addColoredTriangle(waterVertices, wtr, wbl, wbr, waterColor);
    int waterCount = (int)(waterVertices.size() / 9);

    unsigned int waterVAO, waterVBO;
    createMesh(waterVertices, waterVAO, waterVBO);

    //Oggetti della scena (alberi e rocce): un unico vettore di vertici
    std::vector<float> objVertices;
    
    std::srand(42); //seme fisso con scena identica ogni volta

    //Alberi
    float cell = 2.0f / (N - 1); //Lato di una cella della griglia
    for(int z = 0; z < N - 1; z++){
        for(int x = 0; x < N - 1; x++){
            //Punto casuale dentro la cella
            float fx = (float)x / (N - 1) * 2.0f - 1.0f + randf() * cell;
            float fz = (float)z / (N - 1) * 2.0f - 1.0f + randf() * cell;
            float h = terrainHeight(fx, fz);

            //Solo sull'erba (stesse soglie di altitudeColor), con probabilità 30%
            if(h > 0.05f && h < 0.20f && randf() < 0.3f){
                float scale = 0.8f + 0.6f * randf(); //alberi di altezze diverse
                addTree(objVertices, glm::vec3(fx, h - 0.02f, fz), scale);
            }
        }
    }

    //Rocce
    for(int z = 0; z < N - 1; z++){
        for(int x = 0; x < N - 1; x++){
            float fx = (float)x / (N - 1) * 2.0f - 1.0f + randf() * cell;
            float fz = (float)z / (N - 1) * 2.0f - 1.0f + randf() * cell;
            float h = terrainHeight(fx, fz);

            //sopra l'acqua, con probabilità del 8%
            if(h > 0.0f && randf() < 0.08f){
                float s = 0.04f + 0.04f * randf();
                glm::vec3 size(s * 1.5f, s, s * 1.2f); //altezza minore della larghezza
                addRock(objVertices, glm::vec3(fx, h + s * 0.2f, fz),
                        size, randf() * 2.0f * PI, glm::vec3(0.5f, 0.5f, 0.5f));
            }
        }
    }

    int objCount = (int)(objVertices.size() / 9);

    unsigned int objVAO, objVBO;
    createMesh(objVertices, objVAO, objVBO);

    //Costruzione matrici
    glm::mat4 model = glm::mat4(1.0f);
    
    //Stato della camera più alta e più indietro
    glm::vec3 cameraPos(0.0f, 8.0f, 14.0f);
    float yaw = -90.0f; //gradi:  -90 = guarda verso -Z
    float pitch = -30.f; //gradi: negativo = guarda verso il basso 

    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        800.0f / 600.0f,
        0.1f,
        100.0f
    );
    
    unsigned int shaderProgram = createProgram();
    glm::vec3 skyColor(0.6f, 0.75f, 0.9f); //azzurro chiaro
    glClearColor(skyColor.r, skyColor.g, skyColor.b, 1.0f);

    //Aggiunta wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    int mvpLoc = glGetUniformLocation(shaderProgram, "MVP");

    int lightLoc = glGetUniformLocation(shaderProgram, "lightDir");
    glm::vec3 lightDir = glm::normalize(glm::vec3(0.7f, -1.0f, -0.3f));

    glUseProgram(shaderProgram);
    glUniform3fv(lightLoc, 1, glm::value_ptr(lightDir));

    int camLoc = glGetUniformLocation(shaderProgram, "cameraPos");
    int fogColorLoc = glGetUniformLocation(shaderProgram, "fogColor");
    int fogStartLoc = glGetUniformLocation(shaderProgram, "fogStart");
    int fogEndLoc = glGetUniformLocation(shaderProgram, "fogEnd");

    glUniform3fv(fogColorLoc, 1, glm::value_ptr(skyColor));
    glUniform1f(fogStartLoc, 8.0f);
    glUniform1f(fogEndLoc, 30.0f);


    sf::Clock clock;
    float speed = 6.0f; //al secondo

    //cursore
    window.setMouseCursorVisible(false);
    sf::Vector2i center(400, 300);
    sf::Mouse::setPosition(center, window);
    float sensitivity = 0.05f;

    //Loop principale
    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();
            if (const auto* key = event->getIf<sf::Event::KeyPressed>())
                if (key->code == sf::Keyboard::Key::Escape)
                    window.close();
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //Movimento con frecce
        float rotSpeed = 60.f; //gradi al secondo
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) yaw -= rotSpeed * dt;
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) yaw += rotSpeed * dt;
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) pitch += rotSpeed * dt;
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) pitch -= rotSpeed * dt;

        //Mouse look
        if(window.hasFocus()){
            sf::Vector2i mouse = sf::Mouse::getPosition(window);
            yaw += (mouse.x - center.x) * sensitivity;
            pitch -= (mouse.y - center.y) * sensitivity; //y dello schermo va verso il basso
            sf::Mouse::setPosition(center, window);
        }

        //Limiti di pitch
        if(pitch > 89.0f) pitch = 89.0f;
        if(pitch < -89.0f) pitch = -89.0f;

        //Direzione in cui guarda la camera, ricavata da yaw e pitch
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(front);

        glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0, 1, 0)));

        //Movimento da tastiera con dt
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) cameraPos += front * speed * dt;
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) cameraPos -= front * speed * dt;
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) cameraPos += right * speed * dt;
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) cameraPos -= right * speed * dt;

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + front, glm::vec3(0, 1, 0));
        glm::mat4 MVP = projection * view * model;

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(MVP));
        glUniform3fv(camLoc, 1, glm::value_ptr(cameraPos));

        glDrawArrays(GL_TRIANGLES, 0, vertexCount);

        glBindVertexArray(waterVAO);
        glDrawArrays(GL_TRIANGLES, 0, waterCount); 

        glBindVertexArray(objVAO);
        glDrawArrays(GL_TRIANGLES, 0, objCount);
        
        window.display();
    }

    //Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    glDeleteVertexArrays(1, &waterVAO);
    glDeleteBuffers(1, &waterVBO);
    glDeleteVertexArrays(1, &objVAO);
    glDeleteBuffers(1, &objVBO);
    return 0;
}