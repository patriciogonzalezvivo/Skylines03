//
//  SketchCity.cpp
//
//  Created by Patricio Gonzalez Vivo on 9/12/13.
//
//

#include "SketchCity.h"

void SketchCity::selfSetup(){
    ofSetVerticalSync(true);
    
    //  Point Clouds
    //
    pointsShader.load(getDataPath()+"shaders/points");
    
    //  Sprites
    //
    spriteShader.load(getDataPath()+"shaders/sprites");
    ofDisableArbTex();
    for (int i = 0; i < 5; i++) {
        ofLoadImage(spriteTexture[i], getDataPath()+"images/00.png");//"+ofToString(i,2,'0')+".png");
    }
    ofEnableArbTex();
    
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
    
    //  Pano Post-Process
    //
    ditherImage.allocate(512,256,OF_IMAGE_GRAYSCALE);
    panoTexture.allocate(512,256);
    panoPixels.allocate(512,256,OF_IMAGE_COLOR);
    edge.loadFrag(getDataPath()+"shaders/edge.frag");
    
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
    guiAdd(edge);
    guiAdd(pointsShader);
    guiAdd(spriteShader);
}

void SketchCity::selfSetupSystemGui(){
    sysGui->addIntSlider("Max_Dist", 0, 500, &maxDistance);
    sysGui->addSpacer();
    sysGui->addIntSlider("Max_Brightness", 0, 255, &maxBrightnest);
    sysGui->addSpacer();
    sysGui->addToggle("Dithering", &bDither);
    sysGui->addToggle("Filter_floor", &bFilterFloor);
    
    sysGui->addLabel("PCL");
    sysGui->addIntSlider("pcl_k",1,100,&pcl_k);
    sysGui->addSlider("pcl_ml",0.01,5.0,&pcl_ml);
    sysGui->addToggle("pcl_filder",&bPCLFilter);
    
    sysGui->addSpacer();
    sysGui->addToggle("Scrap",&bScrap);
}

void SketchCity::guiSystemEvent(ofxUIEventArgs &e){
    string name = e.widget->getName();
}

void SketchCity::selfSetupRenderGui(){
    rdrGui->addToggle("Show_Pano", &bShowPano);
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
    cout << _sv.getWidth() << "," << _sv.getHeight() << " at zoom " << _sv.getZoom() << endl;
    if (it == loaded.end()){
        loaded[id] = loc;
        for (int i = 0; i < _sv.links.size(); i++) {
            buffer.push_back(_sv.links[i].pano_id);
        }
        
        ofVboMesh tmpMesh;
        tmpMesh.setMode(OF_PRIMITIVE_POINTS);
        
        int mapWidth = _sv.getDepthMapWidth();
        int mapHeight = _sv.getDepthMapHeight();
        
        ofQuaternion ang_offset;
        ang_offset.makeRotate(180-_sv.getDirection(), 0, 0, 1);
        float tiltAngle = _sv.getTiltYaw()*DEG_TO_RAD;
        ofQuaternion tilt_offset;
        tilt_offset.makeRotate(_sv.getTiltPitch(), cos(tiltAngle), sin(-tiltAngle), 0);
        
        panoTexture.begin();
        edge.begin();
        sv.getTextureReference().draw(0,0,panoTexture.getWidth(),panoTexture.getHeight());
        edge.end();
        panoTexture.end();
        panoTexture.getTextureReference().readToPixels(panoPixels);
        
        depthImage = _sv.getDepthMap();
        
        vector<bool> data;
        if(bDither){
            int GrayArrayLength = mapWidth * mapHeight;
            unsigned char * GrayArray = new unsigned char[GrayArrayLength];
            memset(GrayArray,0,GrayArrayLength);
            for (int y = 0; y < mapHeight;y++) {
                for (int x = 0; x < mapWidth; x++) {
                    
                    int pixelCt = 0;
                    float brightTot = 0;
                    
                    ofColor c = panoPixels.getColor(x, y);
                    float brightTemp = c.getBrightness();
                    
                    // Brightness correction curve:
                    brightTemp =  sqrt(255) * sqrt (brightTemp);
                    if (brightTemp > 255) brightTemp = 255;
                    if (brightTemp < 0) brightTemp = 0;
                    
                    int darkness = 255 - floor(brightTemp);
                    
                    int idx = y*mapWidth+x;
                    darkness += GrayArray[idx];
                    
                    if( darkness >= maxBrightnest){
                        darkness -= maxBrightnest;
                        data.push_back(true);
                    } else {
                        data.push_back(false);
                    }
                    
                    int darkn8 = round(darkness / 8);
                    
                    // Atkinson dithering algorithm:  http://verlagmartinkoch.at/software/dither/index.html
                    // Distribute error as follows:
                    //     [ ]  1/8  1/8
                    //1/8  1/8  1/8
                    //     1/8
                    
                    if ((idx + 1) < GrayArrayLength)
                        GrayArray[idx + 1] += darkn8;
                    if ((idx + 2) < GrayArrayLength)
                        GrayArray[idx + 2] += darkn8;
                    if ((idx + mapWidth - 1) < GrayArrayLength)
                        GrayArray[idx + mapWidth - 1] += darkn8;
                    if ((idx + mapWidth) < GrayArrayLength)
                        GrayArray[idx + mapWidth] += darkn8;
                    if ((idx + mapWidth + 1) < GrayArrayLength)
                        GrayArray[idx + mapWidth + 1 ] += darkn8;
                    if ((idx + 2 * mapWidth) < GrayArrayLength)
                        GrayArray[idx + 2 * mapWidth] += darkn8;
                }
            }
            delete []GrayArray;
            
            ofPixels ditherPixels;
            ditherPixels.allocate(mapWidth, mapHeight, 1);
            for(int i = 0; i < data.size();i++){
                ditherPixels.setColor(i, data[i]?0:255);
            }
            ditherImage.setFromPixels(ditherPixels);
        }
        
        for (int x = 0; x < mapWidth; x++) {
            for (unsigned int y = 0; y < mapHeight; y++) {
                
                float rad_azimuth = x / (float) (mapWidth - 1.0f) * TWO_PI;
                float rad_elevation = y / (float) (mapHeight - 1.0f) * PI;
                
                ofPoint pos;
                pos.x = sin(rad_elevation) * sin(rad_azimuth);
                pos.y = sin(rad_elevation) * cos(rad_azimuth);
                pos.z = cos(rad_elevation);
                
                int index = y*mapWidth+x;
                int depthMapIndex = _sv.depthmapIndices[index];
                if (depthMapIndex != 0){
                    DepthMapPlane plane = _sv.depthmapPlanes[depthMapIndex];
                    double distance = -plane.d/(plane.x * pos.x + plane.y * pos.y + -plane.z * pos.z);
                    float brigtness = panoPixels.getColor(x,y).getBrightness();
                    
                    bool bDo = true;
                    if(bDither){
                        bDo = data[index];
                    } else {
                        if(maxBrightnest>0){
                            if(brigtness > maxBrightnest){
                                bDo = false;
                            }
                        }
                    }
                    
                    if(maxDistance>0){
                        if(distance>maxDistance){
                            bDo = false;
                        }
                    }
                    
                    if(bDo){
                        ofPoint vertex;
                        vertex = ang_offset * tilt_offset * pos;
                        vertex *= distance;
                        vertex += _center+ofPoint(0,0,_sv.getGroundHeight());//+_sv.getElevation());
                        
                        if(bFilterFloor){
                            if(ofPoint(plane.x,plane.y,plane.z).dot(ofPoint(0,0,-1))<0.5){
                                tmpMesh.addColor(panoPixels.getColor(x,y));
                                tmpMesh.addVertex(vertex);
                            }
                        } else {
                            tmpMesh.addColor(panoPixels.getColor(x,y));
                            tmpMesh.addVertex(vertex);
                        }
                        
//                        tmpMesh.addColor(panoPixels.getColor(x,y));
//                        mesh.addNormal(ofPoint(plane.x,plane.y,plane.z));
//                        tmpMesh.addVertex(vertex);
                    }
                }
            }
        }
        
        //  Downsample
        //
        if(bPCLFilter){
            ofxPCL::ColorPointCloud cloud = ofxPCL::toPCL<ofxPCL::ColorPointCloud>(tmpMesh);
            ofxPCL::statisticalOutlierRemoval(cloud, pcl_k, pcl_ml);
            
//            float def = 0.001f;
//            ofxPCL::downsample(cloud, ofVec3f(def,def,def));
            tmpMesh.clear();
            tmpMesh = ofxPCL::toOF(cloud);

        }
        
        mesh.addColors(tmpMesh.getColors());
        mesh.addVertices(tmpMesh.getVertices());
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
    
    if(spriteShader.bEnable){
        ofEnableAlphaBlending();
        ofDisableArbTex();
        glDepthMask(GL_FALSE);
        glEnable(GL_NORMALIZE);
        ofEnablePointSprites();
        
        spriteShader.begin();
        spriteShader.getShader().setUniformTexture("tex0",spriteTexture[0],0);
        spriteShader.getShader().setUniformTexture("tex1",spriteTexture[1],1);
        spriteShader.getShader().setUniformTexture("tex2",spriteTexture[2],2);
        spriteShader.getShader().setUniformTexture("tex3",spriteTexture[3],3);
        spriteShader.getShader().setUniformTexture("tex4",spriteTexture[4],4);
        mesh.drawVertices();
        spriteShader.end();
        
        ofDisablePointSprites();
        glDisable(GL_NORMALIZE);
        glDepthMask(GL_TRUE);
        ofEnableArbTex();
    } else {
        ofPushStyle();
        ofPushMatrix();
        glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
        pointsShader.begin();
        mesh.drawVertices();
        pointsShader.end();
        glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);
        ofPopMatrix();
        ofPopStyle();
    }
    
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
        
        if(bShowPano){
            ofPushMatrix();
            ofTranslate(ofPoint(ofGetWidth()-panoTexture.getWidth()-10,uiMap.getHeight()+20));
            ofPushMatrix();
            ofScale(0.5, 0.5);
            panoTexture.draw(0,0);
            depthImage.draw(panoTexture.getWidth(),0);
            if(bDither){
                ditherImage.draw(-panoTexture.getWidth(),0);
            }
            ofPopMatrix();
            
            ofPopMatrix();
        }
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