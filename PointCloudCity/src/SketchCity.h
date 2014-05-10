//
//  SketchCity.h
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

#include "ofxCv.h"

struct Line{
    ofPoint a,b;
};

class SketchCity : public UI3DProject {
public:
    
    string getSystemName(){return "SketchCity";}
    
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
    
    cv::Mat         image, gray, canny;
    float           cannyThreshold1, cannyThreshold2;
    int             maxDistance;
    
    vector<ofPolyline>  getPaths(ofPixels& img, float minGapLength = 2, int minPathLength = 0);
    vector<ofPolyline>  contourLines;
    float           minPathLength, minGapLength;
    
    ofPoint         getVertex(ofVec2f _pos);
    
    vector<Line>    lines;
    float           houghtMinLinLenght,houghtMaxLineGap;
    bool            bScrap, bShowPano;
    
    
    
    //  Point Cloud
    //
    void        addLook(ofPoint _center);
    map<string,Location> loaded;
    ofVboMesh   mesh;
};
