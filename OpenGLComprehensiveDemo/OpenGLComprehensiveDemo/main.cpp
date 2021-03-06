//
//  main.cpp
//  OpenGLComprehensiveDemo
//
//  Created by trs on 2020/7/20.
//  Copyright © 2020 Ctair. All rights reserved.
//

#include "GLTools.h"
#include "GLShaderManager.h"
#include "GLFrustum.h"
#include "GLBatch.h"
#include "GLMatrixStack.h"
#include "GLGeometryTransform.h"
#include "StopWatch.h"

#include <math.h>
#include <stdio.h>

#ifdef __APPLE__
#include <glut/glut.h>
#else
#define FREEGLUT_STATIC
#include <GL/glut.h>
#endif

GLShaderManager        shaderManager;            // 着色器管理器
GLMatrixStack        modelViewMatrix;        // 模型视图矩阵堆栈
GLMatrixStack        projectionMatrix;        // 投影矩阵堆栈
GLFrustum            viewFrustum;            // 视景体
GLGeometryTransform    transformPipeline;        // 几何图形变换管道

GLTriangleBatch     torusBatch;             //大球
GLTriangleBatch     sphereBatch;            //小球
GLBatch             floorBatch;          //地板

//角色帧 照相机角色帧
GLFrame   cameraFrame;
GLFrame  objectFrame;

//**4、添加附加随机球
#define NUM_SPHERES 50
GLFrame spheres[NUM_SPHERES];

void SetupRC(){
    //1. 初始化
    glClearColor(0, 0, 0, 1);
    shaderManager.InitializeStockShaders();
    glEnable(GL_DEPTH_TEST);
    
    //3. 地板数据(物体坐标系)
    floorBatch.Begin(GL_LINES, 324);
    for(GLfloat x = -20.0; x <= 20.0f; x+= 0.5) {
        floorBatch.Vertex3f(x, -0.55f, 20.0f);
        floorBatch.Vertex3f(x, -0.55f, -20.0f);
        floorBatch.Vertex3f(20.0f, -0.55f, x);
        floorBatch.Vertex3f(-20.0f, -0.55f, x);
    }
    floorBatch.End();
    
    //4. 设置一个球体(基于gltools模型)
    gltMakeSphere(torusBatch, 0.4f, 40, 60);
    
    //5. 绘制小球;
    gltMakeSphere(sphereBatch, 0.2f, 40, 80);
    //6. 随机位置放置小球球
    for (int i = 0; i < NUM_SPHERES; i++) {

        //y轴不变，X,Z产生随机值
        GLfloat x = ((GLfloat)((rand() % 400) - 200 ) * 0.1f);
        GLfloat z = ((GLfloat)((rand() % 400) - 200 ) * 0.1f);

        //在y方向，将球体设置为0.0的位置，这使得它们看起来是飘浮在眼睛的高度
        //对spheres数组中的每一个顶点，设置顶点数据
        spheres[i].SetOrigin(x, 0.0f, z);
    }
    
}

//进行调用以绘制场景
void RenderScene(void)
{
    //3.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    //1. 颜色(地板,大球颜色,小球颜色)
    // 网格层面颜色
    static GLfloat vFloorColor[] = {0.0f,1.0f,0.0f,1.0f};
    //大球颜色
    static GLfloat vMaxBallColor[] = {0.85f, 0.85f, 0.85f, 1.0f};
    static GLfloat vMinBallColor[] = {0.9f, 0.1f, 0.4f, 1.0f};
    
    
    //2. 动画
    static CStopWatch rotTimmer;
    float yRot = rotTimmer.GetElapsedSeconds() * 60.0f;

    // 压栈（在保留操作之后，push还原）
    modelViewMatrix.PushMatrix();
    
    M3DMatrix44f mCamera;
    cameraFrame.GetCameraMatrix(mCamera);
    modelViewMatrix.PushMatrix(mCamera);
    
    
    //网格层面绘制;
    shaderManager.UseStockShader(GLT_SHADER_FLAT, transformPipeline.GetModelViewProjectionMatrix(), vFloorColor);
    floorBatch.Draw();
    
    
    //5. 设置点光源位置
    M3DVector4f vLightPos = {0,10,5,1};

    //6. 使得整个大球往里平移3.0
    modelViewMatrix.Translate(0.0f, 0.0f, -4.0f);

    //7. 大球
    // 再次压栈，防止每次renderSence重复操作平移矩阵
    modelViewMatrix.PushMatrix();
    modelViewMatrix.Rotate(yRot, 0, 1, 0);
    
    
    shaderManager.UseStockShader(GLT_SHADER_POINT_LIGHT_DIFF, transformPipeline.GetModelViewMatrix(), transformPipeline.GetProjectionMatrix(), vLightPos, vMaxBallColor);
    torusBatch.Draw();
    modelViewMatrix.PopMatrix();


    //8. 小球
    for (int i = 0; i < NUM_SPHERES; i++) {
        modelViewMatrix.PushMatrix();
        modelViewMatrix.MultMatrix(spheres[i]);
        
        shaderManager.UseStockShader(GLT_SHADER_POINT_LIGHT_DIFF, transformPipeline.GetModelViewMatrix(), transformPipeline.GetProjectionMatrix(), vLightPos, vMinBallColor);
        
        sphereBatch.Draw();
        modelViewMatrix.PopMatrix();
    }

//    //9.让一个小球围着大球公转;
    //小球的旋转方向为大球的自转的逆向
    modelViewMatrix.Rotate(yRot * -2.5f, 0, 1, 0);
    // 大球的半径为0.4，小球的半径为0.2，所有小球需要从在x轴的方向做平移变换大于0.6
    modelViewMatrix.Translate(0.7f, 0.0f, 0.0f);

    shaderManager.UseStockShader(GLT_SHADER_POINT_LIGHT_DIFF,transformPipeline.GetModelViewMatrix(),transformPipeline.GetProjectionMatrix(),vLightPos,vMinBallColor);
    sphereBatch.Draw();

    // 对应两次pop
    modelViewMatrix.PopMatrix();
    modelViewMatrix.PopMatrix();
    glutSwapBuffers();
    glutPostRedisplay();
}

//屏幕更改大小或已初始化
void ChangeSize(int nWidth, int nHeight){
    //1. 设置视口
    glViewport(0, 0, nWidth, nHeight);

    //2. 创建投影矩阵
    viewFrustum.SetPerspective(35.0f, float(nWidth)/float(nHeight), 1.0f, 100.0f);
    projectionMatrix.LoadMatrix(viewFrustum.GetProjectionMatrix());
    
    //3. 变换管道设置2个矩阵堆栈(管理)
    transformPipeline.SetMatrixStacks(modelViewMatrix, projectionMatrix);
}

void SpeacialKeys(int key,int x,int y){
    //定义步长
    float linear = 0.1f;
    //定义每次旋转的角度
    float angular = float(m3dDegToRad(6.0f));
    
    if (key == GLUT_KEY_UP) {
        cameraFrame.MoveForward(linear);
    }
    if (key == GLUT_KEY_DOWN) {
        cameraFrame.MoveForward(-linear);
    }
    //只围绕y轴转，模拟人眼同一水平旋转效果
    if (key == GLUT_KEY_LEFT) {
        cameraFrame.RotateWorld(angular, 0, 1, 0);
    }
    if (key == GLUT_KEY_RIGHT) {
        cameraFrame.RotateWorld(-angular, 0, 1, 0);
    }
    /// 说话的人呢？
}

int main(int argc, char* argv[])
{
    gltSetWorkingDirectory(argv[0]);
    
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800,600);
    
    glutCreateWindow("OpenGL SphereWorld");
    
    glutReshapeFunc(ChangeSize);
    glutDisplayFunc(RenderScene);
    glutSpecialFunc(SpeacialKeys);
    
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(err));
        return 1;
    }
    
    
    SetupRC();
    glutMainLoop();
    return 0;
}
