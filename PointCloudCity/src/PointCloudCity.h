//
//  PointCloudCity.h
//
//  Created by Patricio Gonzalez Vivo on 9/12/13.
//
//

#pragma once

#include "UIProject.h"

#include "UIShader.h"
#include "UI3DGrid.h"

#include "UIMap.h"
#include "StreetView.h"

#include "ofxPCL.h"

class PointCloudCity : public UI3DProject {
public:
    
    string getSystemName(){return "PointCloudCity";}
    
    void selfSetupGuis();
    
    void selfSetupSystemGui();
    void guiSystemEvent(ofxUIEventArgs &e);
    
    void selfSetupRenderGui();
    void guiRenderEvent(ofxUIEventArgs &e);
    
    void selfSetup();
    void selfUpdate();
    void selfDraw();
    void selfDrawOverlay();

    void selfWindowResized(ofResizeEventArgs & args);
    
    void selfKeyPressed(ofKeyEventArgs & args);
    void selfKeyReleased(ofKeyEventArgs & args);
    
    void selfMouseDragged(ofMouseEventArgs& data);
    void selfMouseMoved(ofMouseEventArgs& data);
    void selfMousePressed(ofMouseEventArgs& data);
    void selfMouseReleased(ofMouseEventArgs& data);

protected:
    UI3DGrid    grid;
    
    //  Map
    //
    UIMap       uiMap;
    Location    firstLocation;
    bool        bFirstDef;
    
    //  Google Street View
    //
    StreetView  sv;
    vector<string> buffer;
    bool        bScrap;

    //  Post processing pano image
    //
    UIShader    edge;
    ofFbo       panoTexture;
    ofPixels    panoPixels;
    ofImage     ditherImage;
    ofImage     depthImage;
    bool        bDither;
    bool        bFilterFloor;
    int         maxBrightnest;
    int         maxDistance;
    bool        bShowPano;
    
    //  Point Cloud
    //
    void        addLook(StreetView &_sv, ofPoint _center);
    map<string,Location> loaded;
    ofVboMesh   mesh;
    int         pcl_k;
    float       pcl_ml;
    bool        bPCLFilter;
    
    UIShader    pointsShader;
    UIShader    spriteShader;
    ofTexture   spriteTexture[5];
};