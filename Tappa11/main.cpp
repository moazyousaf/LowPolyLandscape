#include <SFML/Window.hpp>
#include <glad/glad.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>

// --- Costanti globali ---
const int N = 40; //griglia 40x40
const float SIZE = 10.0f;

//Finestra
const unsigned int WIDTH = 800;
const unsigned int HEIGHT = 600;

//Acqua
const float WATER_LEVEL = 0.0f;
const float WATER_SIZE = SIZE * 1.2f;
const float WATER_ALPHA = 0.6f;

//Nebbia
const float FOG_START = 8.0f;
const float FOG_END = 30.0f;

//Probabilità di generazione degli oggetti
const float TREE_CHANCE = 0.15f;
const float ROCK_CHANCE = 0.05f;

//Camera
const float CAMERA_SPEED = 6.0f;
const float MOUSE_SENSITIVITY = 0.05f;
const float EYE_HEIGHT = 0.5f;


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
uniform float alpha;
out vec4 outColor;
void main() {
    vec3 n = normalize(fragNormal);

    float ambient = 0.2;
    float diffuse = max(dot(n, -lightDir), 0.0);
    vec3 lit = fragColor * (ambient + diffuse);

    //Nebbia: 0 = nessuna, 1 = solo nebbia
    float dist = length(fragPos - cameraPos);
    float fogFactor = clamp((dist - fogStart) / (fogEnd - fogStart), 0.0, 1.0);

    outColor = vec4(mix(lit, fogColor, fogFactor), alpha);
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
    if(h < -0.5f) return glm::vec3(0.2f, 0.3f, 0.35f); //Fondale profondo
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

//addTriangle con colore arbitrario
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

//Punto casuale dentro la cella della griglia (x, z)
glm::vec2 randomPointInCell(int x, int z){
    float cell = 2.0f * SIZE / (N - 1); //Lato di una cella della griglia
    float fx = -SIZE + x * cell + randf() * cell;
    float fz = -SIZE + z * cell + randf() * cell;

    return glm::vec2(fx, fz);
}

//Triangoli del terreno
std::vector<float> buildTerrain(){
    std::vector<float> v;
    for(int z = 0; z < N - 1; z++){
        for(int x = 0; x < N - 1; x++){
            glm::vec3 tl = gridPoint(x, z);
            glm::vec3 tr = gridPoint(x + 1, z);
            glm::vec3 bl = gridPoint(x, z + 1);
            glm::vec3 br = gridPoint(x + 1, z + 1);
            
            addTriangle(v, tl, bl, tr); //triangolo 1
            addTriangle(v, tr, bl, br); //triangolo 2
            
        }
    }
    return v;
}

//Piano d'acqua
std::vector<float> buildWater(){
    float w = WATER_SIZE;
    glm::vec3 waterColor(0.12f, 0.38f, 0.72f);

    glm::vec3 wtl(-w, WATER_LEVEL, -w);
    glm::vec3 wtr( w, WATER_LEVEL, -w);
    glm::vec3 wbl(-w, WATER_LEVEL,  w);
    glm::vec3 wbr( w, WATER_LEVEL,  w);

    std::vector<float> v;
    addColoredTriangle(v, wtl, wbl, wtr, waterColor);
    addColoredTriangle(v, wtr, wbl, wbr, waterColor);

    return v;
}

//Alberi e rocce
std::vector<float> buildObjects(){
    std::vector<float> v;
    std::srand(42); //seme fisso con scena identica ogni volta

    //Alberi
    for(int z = 0; z < N - 1; z++){
        for(int x = 0; x < N - 1; x++){
            //Punto casuale dentro la cella
            glm::vec2 p = randomPointInCell(x, z);
            float h = terrainHeight(p.x, p.y);

            //Solo sull'erba (stesse soglie di altitudeColor), con probabilità 15%
            if(h > 0.5f && h < 2.0f && randf() < TREE_CHANCE){
                float scale = 4.0f + 2.0f * randf(); //alberi di altezze diverse
                addTree(v, glm::vec3(p.x, h - 0.01f, p.y), scale);
            }
        }
    }

    //Rocce
    for(int z = 0; z < N - 1; z++){
        for(int x = 0; x < N - 1; x++){
            glm::vec2 p = randomPointInCell(x, z);
            float h = terrainHeight(p.x, p.y);

            //sopra l'acqua, con probabilità del 5%
            if(h > WATER_LEVEL && randf() < ROCK_CHANCE){
                float s = 0.15f + 0.15f * randf(); //dimensione delle rocce tra 0.15 e 0.3
                glm::vec3 size(s * 1.5f, s, s * 1.2f); //altezza minore della larghezza
                addRock(v, glm::vec3(p.x, h + s * 0.2f, p.y),
                        size, randf() * 2.0f * PI, glm::vec3(0.5f, 0.5f, 0.5f));
            }
        }
    }
    return v;
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
        sf::VideoMode({WIDTH, HEIGHT}),
        "Low-Poly Landscape - Tappa 11",
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

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //formula con alpha
    
    std::vector<float> vertices = buildTerrain();
    std::vector<float> waterVertices = buildWater();
    std::vector<float> objVertices = buildObjects();

    int vertexCount = (int)(vertices.size() / 9); //9 float per vertice
    int waterCount = (int)(waterVertices.size() / 9);
    int objCount = (int)(objVertices.size() / 9);

    unsigned int VAO, VBO, waterVAO, waterVBO, objVAO, objVBO;
    createMesh(vertices, VAO, VBO);
    createMesh(waterVertices, waterVAO, waterVBO);
    createMesh(objVertices, objVAO, objVBO);
    std::cout << vertexCount << " " << waterCount << " " << objCount << std::endl; //check

    //Costruzione matrici
    glm::mat4 model = glm::mat4(1.0f);
    
    //Stato della camera più alta e più indietro
    glm::vec3 cameraPos(0.0f, 8.0f, 9.0f);
    float yaw = -90.0f; //gradi:  -90 = guarda verso -Z
    float pitch = -30.f; //gradi: negativo = guarda verso il basso 

    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        (float)WIDTH / (float)HEIGHT,
        0.1f,
        100.0f
    );
    
    unsigned int shaderProgram = createProgram();
    glm::vec3 skyColor(0.6f, 0.75f, 0.9f); //azzurro chiaro
    glClearColor(skyColor.r, skyColor.g, skyColor.b, 1.0f);

    int mvpLoc = glGetUniformLocation(shaderProgram, "MVP");

    int lightLoc = glGetUniformLocation(shaderProgram, "lightDir");
    glm::vec3 lightDir = glm::normalize(glm::vec3(0.7f, -1.0f, -0.3f));

    glUseProgram(shaderProgram);
    glUniform3fv(lightLoc, 1, glm::value_ptr(lightDir));

    int camLoc = glGetUniformLocation(shaderProgram, "cameraPos");
    int fogColorLoc = glGetUniformLocation(shaderProgram, "fogColor");
    int fogStartLoc = glGetUniformLocation(shaderProgram, "fogStart");
    int fogEndLoc = glGetUniformLocation(shaderProgram, "fogEnd");
    int alphaLoc = glGetUniformLocation(shaderProgram, "alpha");

    glUniform3fv(fogColorLoc, 1, glm::value_ptr(skyColor));
    glUniform1f(fogStartLoc, FOG_START);
    glUniform1f(fogEndLoc, FOG_END);


    sf::Clock clock;

    //cursore
    window.setMouseCursorVisible(false);
    sf::Vector2i center(WIDTH / 2, HEIGHT / 2);
    sf::Mouse::setPosition(center, window);

    bool wireframe = false;
    bool fogOn = true;

    //Loop principale
    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();
            if (const auto* key = event->getIf<sf::Event::KeyPressed>()){
                if (key->code == sf::Keyboard::Key::Escape)
                    window.close();
                if(key->code == sf::Keyboard::Key::F){
                    wireframe = !wireframe;
                    glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
                }
                if(key->code == sf::Keyboard::Key::N){
                    fogOn = !fogOn;
                    glUniform1f(fogStartLoc, fogOn ? FOG_START  : 10000.0f);
                    glUniform1f(fogEndLoc,   fogOn ? FOG_END : 20000.0f);
                }
            }
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
            yaw += (mouse.x - center.x) * MOUSE_SENSITIVITY;
            pitch -= (mouse.y - center.y) * MOUSE_SENSITIVITY; //y dello schermo va verso il basso
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
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) cameraPos += front * CAMERA_SPEED * dt;
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) cameraPos -= front * CAMERA_SPEED * dt;
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) cameraPos += right * CAMERA_SPEED * dt;
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) cameraPos -= right * CAMERA_SPEED * dt;

        //Limite camera dentro il terreno e sopra la superficie
        cameraPos.x = glm::clamp(cameraPos.x, -SIZE, SIZE);
        cameraPos.z = glm::clamp(cameraPos.z, -SIZE, SIZE);

        float groundY = std::max(terrainHeight(cameraPos.x, cameraPos.z), WATER_LEVEL);
        float minY = groundY + EYE_HEIGHT;
        if(cameraPos.y < minY) cameraPos.y = minY;     

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + front, glm::vec3(0, 1, 0));
        glm::mat4 MVP = projection * view * model;

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(MVP));
        glUniform3fv(camLoc, 1, glm::value_ptr(cameraPos));
        glUniform1f(alphaLoc, 1.0f); //opaco

        glDrawArrays(GL_TRIANGLES, 0, vertexCount);

        glBindVertexArray(objVAO);
        glDrawArrays(GL_TRIANGLES, 0, objCount);

        glUniform1f(alphaLoc, WATER_ALPHA); //acqua semitrasparente
        glBindVertexArray(waterVAO);
        glDrawArrays(GL_TRIANGLES, 0, waterCount); 
        
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