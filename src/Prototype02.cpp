//
//  Prototype02.cpp
//
//  Created by Patricio Gonzalez Vivo on 9/12/13.
//
//

#include "Prototype02.h"

void Prototype02::selfSetup(){
    ofSetVerticalSync(true);

    ofBuffer tripBuffer = ofBufferFromFile("BEACON.txt");
    for (int i = 0; i < tripBuffer.size(); i++) {
        ids.push_back(tripBuffer.getNextLine());
    }
    
    bLoad = true;
    nLoad = 0;
    A.setPanoId(ids[nLoad]);
    B.setPanoId(ids[nLoad+1]);
    
    pointCloudShader.load(getDataPath()+"shaders/points");
}

void Prototype02::selfSetupGuis(){
    backgroundSet(new UIMapBackground());
    cameraSet(new UIGameCam());
    
    guiAdd(grid);
    guiAdd(pointCloudShader);
}

void Prototype02::selfSetupSystemGui(){
    sysGui->addSlider("Angle", 0, 360, &view_ang);
    sysGui->addSlider("Amplitud", 0, 360, &view_amp);
    sysGui->addToggle("Fetch", &bLoad);
}

void Prototype02::guiSystemEvent(ofxUIEventArgs &e){
    string name = e.widget->getName();

    nLoad = 0;
    A.setPanoId(ids[nLoad]);
    B.setPanoId(ids[nLoad+1]);
}

void Prototype02::selfSetupRenderGui(){

}

void Prototype02::guiRenderEvent(ofxUIEventArgs &e){
    string name = e.widget->getName();
}

void Prototype02::selfUpdate(){
    
    if(bLoad){
        texA = A.getTextureReference();
        texB = B.getTextureReference();
        
        if(A.isTextureLoaded()&&B.isTextureLoaded()){
            
            Location locA = A.getLocation();
            Location locB = B.getLocation();
            
            ofPoint posA;
            ofPoint posB;
            
            float direction = locA.getDegTo(locB);
            
            if(nLoad==0){
                mesh.clear();
                mesh.setMode(OF_PRIMITIVE_POINTS);
                
                prevLocation = locA;
                
                posA = ofPoint(0,0);
                prevPos = ofPoint(0,0);
            } else {
                float angleA = prevLocation.getRadTo(locA);//*DEG_TO_RAD;
                float distA = prevLocation.getDistanceTo(locA);
                
                posA.x = distA*cos(angleA);
                posA.y = distA*sin(angleA);
            }
            
            float angleB = prevLocation.getRadTo(locB);//*DEG_TO_RAD;
            float distB = prevLocation.getDistanceTo(locB);
            posB.x = distB*cos(angleB);
            posB.y = distB*sin(angleB);

            posA += prevPos;
            posB += prevPos;
            
            addLook(A,posA,direction+view_ang,view_amp);
            addLook(B,posB,direction+view_ang,view_amp);
            
            prevLocation = locB;
            prevPos = posB;
            if (nLoad+2>=ids.size()){
                bLoad = false;
            } else {
                nLoad+=2;
                A.setPanoId(ids[nLoad]);
                B.setPanoId(ids[nLoad+1]);
            }
        }
    }
}

void Prototype02::addLook(StreetView &_sv, ofPoint _center, float _angle, float _ap){
    float aStart = _angle-_ap*0.5;
    float aEnd = _angle+_ap*0.5;
    
    int mapWidth = _sv.getDepthMapWidth();
    int mapHeight = _sv.getDepthMapHeight();
    
    float maxDistance = 100;
    
    ofQuaternion ang_offset;
    ang_offset.makeRotate(180-_sv.getDirection(), 0, 0, 1);
//    
//    ofQuaternion tilt_offset;
//    tilt_offset.makeRotate(_sv.getTiltPitch(), 0, 1, 0);
    
    ofPixels pixelsPano;
    _sv.getTextureReference().readToPixels(pixelsPano);
    
    for (int a = aStart; a < aEnd; a++) {
        
        int x = ((int)((((360.0*10.0)+a)/360.0)*mapWidth))%mapWidth;
        
        for (unsigned int y = 0; y < mapHeight; y++) {
            
            float rad_azimuth = x / (float) (mapWidth - 1.0f) * TWO_PI;
            float rad_elevation = y / (float) (mapHeight - 1.0f) * PI;

            ofPoint pos;
            pos.x = sin(rad_elevation) * sin(rad_azimuth);
            pos.y = sin(rad_elevation) * cos(rad_azimuth);
            pos.z = cos(rad_elevation);
            
            ofPoint normal;
            ofPoint vertex;
            float distance = 0;
            
            int depthMapIndex = _sv.depthmapIndices[y*mapWidth+x];
            if (depthMapIndex == 0) {
//                distance = maxDistance;
//                normal = pos.getNormalized();
            } else {
                DepthMapPlane plane = _sv.depthmapPlanes[depthMapIndex];
                distance = -plane.d/(plane.x * pos.x + plane.y * pos.y + -plane.z * pos.z);
                normal.set(plane.x,plane.y,plane.z);
                
                if(distance<maxDistance){
                    vertex = ang_offset * pos;
                    vertex *= distance;
                    vertex += _center+ofPoint(0,0,_sv.getGroundHeight());
                    
                    mesh.addColor(pixelsPano.getColor(ofMap(x, 0, mapWidth, 0, pixelsPano.getWidth()),
                                                      ofMap(y, 0, mapHeight, 0, pixelsPano.getHeight())) );
                    mesh.addNormal(normal);
                    mesh.addVertex(vertex);
                }
            }
        }
    }
}

void Prototype02::selfDraw(){
    materials["MATERIAL 1"]->begin();
    ofPushMatrix();
    ofRotate(90, 1, 0, 0);
    
    grid.draw();
    ofPopMatrix();
    
    ofSetColor(255);
    
    ofPushStyle();
    ofPushMatrix();
//    glDepthMask(GL_FALSE);
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
    pointCloudShader.begin();
    pointCloudShader.getShader().setUniform1f("nearClip", 10);//getCameraRef().getNearClip());
    pointCloudShader.getShader().setUniform1f("farClip", 1000);//getCameraRef().getFarClip());
    mesh.drawVertices();
    pointCloudShader.end();
    glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);
//    glDepthMask(GL_TRUE);
    ofPopMatrix();
    ofPopStyle();
    
    materials["MATERIAL 1"]->end();
}

void Prototype02::selfKeyPressed(ofKeyEventArgs & args){
}

void Prototype02::selfKeyReleased(ofKeyEventArgs & args){
}

void Prototype02::selfMouseMoved(ofMouseEventArgs& data){
}

void Prototype02::selfMousePressed(ofMouseEventArgs& data){
}

void Prototype02::selfMouseDragged(ofMouseEventArgs& data){
}

void Prototype02::selfMouseReleased(ofMouseEventArgs& data){
}

void Prototype02::selfWindowResized(ofResizeEventArgs & args){
    
}