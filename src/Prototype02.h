//
//  Prototype02.h
//
//  Created by Patricio Gonzalez Vivo on 9/12/13.
//
//

#pragma once

#include "UIProject.h"

#include "UIShader.h"
#include "UI3DGrid.h"

#include "StreetView.h"

class Prototype02 : public UI3DProject {
public:
    
    string getSystemName(){return "Prototype02";}
    
    void selfSetupGuis();
    
    void selfSetupSystemGui();
    void guiSystemEvent(ofxUIEventArgs &e);
    
    void selfSetupRenderGui();
    void guiRenderEvent(ofxUIEventArgs &e);
    
    void selfSetup();
    void selfUpdate();
    void selfDraw();

    void selfWindowResized(ofResizeEventArgs & args);
    
    void selfKeyPressed(ofKeyEventArgs & args);
    void selfKeyReleased(ofKeyEventArgs & args);
    
    void selfMouseDragged(ofMouseEventArgs& data);
    void selfMouseMoved(ofMouseEventArgs& data);
    void selfMousePressed(ofMouseEventArgs& data);
    void selfMouseReleased(ofMouseEventArgs& data);

protected:
    UI3DGrid    grid;
    
    StreetView  A,B;
    ofTexture   texA,texB;
    
    Location    prevLocation;
    ofPoint     prevPos;
    
    UIShader    pointCloudShader;
    ofVboMesh   mesh;
    
    void        addLook(StreetView &_sv, ofPoint _center,float _angleToLook, float _amplitud);
    
    float       view_ang,view_amp;
    vector<string> ids;
    
    int         nLoad;
    bool        bLoad;
};
