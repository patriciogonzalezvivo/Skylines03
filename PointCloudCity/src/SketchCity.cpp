//
//  SketchCity.cpp
//
//  Created by Patricio Gonzalez Vivo on 9/12/13.
//
//

#include "SketchCity.h"

void SketchCity::selfSetup(){
    ofSetVerticalSync(true);
    
    //  Map
    //
    uiMap.allocate(512.0*1.5, 240*1);
    uiMap.loadMap("Stamen-toner");
    uiMap.setZoom(15);
    
    //  Google Maps
    //
    sv.setZoom(1);
    
    //  First Location
    //
    firstLocation.lat = 0;
    firstLocation.lon = 0;
    bFirstDef = false;
    
    //  Point Cloud Mesh
    //
    mesh.clear();
    mesh.setMode(OF_PRIMITIVE_POINTS);
}

void SketchCity::selfSetupGuis(){
    backgroundSet(new UIMapBackground());
    cameraSet(new UIGameCam());
    guiAdd(uiMap);
    guiAdd(grid);
}

void SketchCity::selfSetupSystemGui(){
    
    sysGui->addSpacer();
    sysGui->addToggle("Scrap",&bScrap);
}

void SketchCity::guiSystemEvent(ofxUIEventArgs &e){
    string name = e.widget->getName();
}

void SketchCity::selfSetupRenderGui(){
}

void SketchCity::guiRenderEvent(ofxUIEventArgs &e){
    string name = e.widget->getName();
}

void SketchCity::selfUpdate(){
    uiMap.update();
    sv.update();
    
    if(sv.isTextureLoaded()&&bScrap){
        Location loc = sv.getLocation();
        ofPoint pos = ofPoint(0,0);
        
        if(loaded.size()==0){
            mesh.clear();
            mesh.setMode(OF_PRIMITIVE_POINTS);
            
            firstLocation = loc;
            bFirstDef = true;
        } else {
            ofPoint mapPos = uiMap.locationPoint(loc);
            ofPoint mapCenter = uiMap.locationPoint(firstLocation);
            ofPoint diff = mapPos-mapCenter;
            
            float angle = atan2(diff.y,diff.x);
            double dist = firstLocation.getDistanceTo(loc);
            
            pos.x = dist*cos(angle);
            pos.y = dist*sin(-angle);
        }
    
        addLook(sv,pos);
        
        if(buffer.size()>0){
            sv.setPanoId(buffer[0]);
            buffer.erase(buffer.begin()+0);
        }
    }
    
}

void SketchCity::addLook(StreetView &_sv, ofPoint _center){
    string id = _sv.getPanoId();
    Location loc = _sv.getLocation();
    
    auto it = loaded.find(id);
    if (it == loaded.end()){
        loaded[id] = loc;
        for (int i = 0; i < _sv.links.size(); i++) {
            buffer.push_back(_sv.links[i].pano_id);
        }
        
    }
}

void SketchCity::selfDraw(){
    materials["MATERIAL 1"]->begin();
    ofPushMatrix();
    ofTranslate(0,0,-25);
    ofRotate(90, 1, 0, 0);
    grid.draw();
    ofPopMatrix();
    
    ofSetColor(255);
    
    mesh.draw();
    
    materials["MATERIAL 1"]->end();
}

void SketchCity::selfDrawOverlay(){
    if(bDebug){
        
        ofPushStyle();
        
        ofPoint mapPos = ofPoint(ofGetWidth()-uiMap.getWidth()-10, +10);
        
        ofPushMatrix();
        ofTranslate(mapPos);
        ofSetColor(255);
        ofEnableBlendMode(OF_BLENDMODE_MULTIPLY);
        uiMap.draw();
        ofDisableBlendMode();
        ofEnableAlphaBlending();
        ofRectangle area = ofRectangle(0,0,uiMap.getWidth(),uiMap.getHeight());
        
        for (std::map<string,Location>::iterator it=loaded.begin(); it!=loaded.end(); ++it){
            ofPoint pos = uiMap.locationPoint(it->second);
            
            if(area.inside(pos)){
                ofPushStyle();
                ofSetColor(255, 0, 0,255);
                ofCircle(pos, 2);
                ofPopStyle();
            }
        }
        
        if(bFirstDef){
            ofPoint pos = camera->getCameraLocation().position;
            ofPoint ori = camera->getCameraLocation().orientation.getEuler();
            
            float dist = pos.length();
            float angle = atan2(pos.y,pos.x);
            
            ofPoint mapCenter = uiMap.locationPoint(firstLocation);
            float scale = firstLocation.getDistanceTo(uiMap.pointLocation(mapCenter+ofPoint(1,0)));
            
            ofPoint camPos;
            camPos.x = mapCenter.x + (dist/scale)*cos(angle);
            camPos.y = mapCenter.y + (dist/scale)*sin(-angle);
            
            if(area.inside(camPos)){
                ofPushStyle();
                ofSetColor(255,0, 0,255);
                ofNoFill();
                ofCircle(camPos, 2);
            
                ofPushMatrix();
                ofTranslate(camPos);
                ofRotate(ori.z,0, 0, -1);
                
                
                float fov = ofDegToRad(camera->getCameraLocation().FOV);
                ofPoint a,b;
                a.x = 10*cos(-PI*0.5+fov*0.5);
                a.y = 10*sin(-PI*0.5+fov*0.5);
                b.x = 10*cos(-PI*0.5-fov*0.5);
                b.y = 10*sin(-PI*0.5-fov*0.5);
                
                ofLine(ofPoint(0,0),a);
                ofLine(ofPoint(0,0),b);
                
                ofPopMatrix();
                ofPopStyle();
            }
        }
        
        
        ofPopMatrix();
        ofPopStyle();
    }
}

void SketchCity::selfKeyPressed(ofKeyEventArgs & args){
    if(args.key == OF_KEY_RETURN){
        buffer.clear();
        sv.setLocation(uiMap.getCenter());
    } else if (args.key == 'p'){
        mesh.clear();
        loaded.clear();
        buffer.clear();
    }
}

void SketchCity::selfKeyReleased(ofKeyEventArgs & args){
}

void SketchCity::selfMouseMoved(ofMouseEventArgs& data){
}

void SketchCity::selfMousePressed(ofMouseEventArgs& data){
    if (bDebug){
        
        ofPoint areaPos = ofPoint(ofGetWidth()-uiMap.getWidth()-10, +10);
        ofRectangle area = ofRectangle(areaPos,uiMap.getWidth(),uiMap.getHeight());
        
        if (area.inside(data)) {
            camera->disableMouseInput();
            
            if(bFirstDef){
                
                CameraLocation camLoc = camera->getCameraLocation();
                ofPoint mouse = data - areaPos;
                Location loc = uiMap.pointLocation(mouse);
                ofPoint mapCenter = uiMap.locationPoint(firstLocation);
                ofPoint diff = mouse-mapCenter;
                
                float angle = atan2(diff.y,diff.x);
                double dist = firstLocation.getDistanceTo(loc);
                
                camLoc.position.x = dist*cos(angle);
                camLoc.position.y = dist*sin(-angle);
                
                camera->setCameraLocation(camLoc);
            }
        }
    }
}

void SketchCity::selfMouseDragged(ofMouseEventArgs& data){
    if (bDebug){
        ofPoint areaPos = ofPoint(ofGetWidth()-uiMap.getWidth()-10, +10);
        ofRectangle area = ofRectangle(areaPos,uiMap.getWidth(),uiMap.getHeight());
        
        if (area.inside(data)) {
            ofPoint mouse = data - areaPos;
            ofPoint pMouse = ofPoint(ofGetPreviousMouseX(),ofGetPreviousMouseY()) - areaPos;
            
            float scale = pow(2.0, uiMap.getZoom());
            ofPoint dMouse = (mouse - pMouse);
            
            if (data.button == 0) {
                uiMap.panBy(dMouse);
            } else if (data.button == 2) {
                if (ofGetKeyPressed( OF_KEY_SHIFT )) {
                    uiMap.rotateBy(dMouse.x < 0 ? M_PI/72.0 : -M_PI/72.0, mouse);
                } else {
                    uiMap.scaleBy(dMouse.y < 0 ? 1.05 : 1.0/1.05, mouse);
                }
            }
            uiMap.setCenter(uiMap.getCenter());
        }
    }
}

void SketchCity::selfMouseReleased(ofMouseEventArgs& data){
    camera->enableMouseInput();
}

void SketchCity::selfWindowResized(ofResizeEventArgs & args){
    
}