//
//  SketchCity.cpp
//
//  Created by Patricio Gonzalez Vivo on 9/12/13.
//
//

#include "SketchCity.h"

using namespace ofxCv;
using namespace cv;

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
    lineMesh.clear();
    lineMesh.setMode(OF_PRIMITIVE_LINES);
}

void SketchCity::selfSetupGuis(){
    backgroundSet(new UIMapBackground());
    cameraSet(new UIGameCam());
    guiAdd(uiMap);
    guiAdd(grid);
}

void SketchCity::selfSetupSystemGui(){
    sysGui->addIntSlider("Max_Dist", 0, 500, &maxDistance);
    sysGui->addSpacer();
    sysGui->addLabel("Canny");
    sysGui->addSlider("Canny_Threshold1", 0.0, 1024, &cannyThreshold1);
    sysGui->addSlider("Canny_Threshold2",0.0,1024,&cannyThreshold2);
    sysGui->addSlider("MinGapLenght", 2.0, 12.0, &minGapLength);
    sysGui->addSlider("MinPathLenght", 0.0, 50.0, &minPathLength);
    
    sysGui->addLabel("HoughLines");
    sysGui->addSlider("HoughtMinLinLength", 1, 10, &houghtMinLinLenght);
    sysGui->addSlider("HoughtMaxLineGap", 1, 10, &houghtMaxLineGap);
    
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
            lineMesh.clear();
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
    
        addLook(pos);
        
        if(buffer.size()>0){
            sv.setPanoId(buffer[0]);
            buffer.erase(buffer.begin()+0);
        }
    }
    
}

void SketchCity::addLook(ofPoint _center){
    string id = sv.getPanoId();
    Location loc = sv.getLocation();
    
    auto it = loaded.find(id);
    if (it == loaded.end()){
        loaded[id] = loc;
        for (int i = 0; i < sv.links.size(); i++) {
            buffer.push_back(sv.links[i].pano_id);
        }
        
        //  Get the pixels
        //
        ofPixels pixels;
        sv.getTextureReference().readToPixels(pixels);
        
        //  Extract the lines using openCv
        //
        image = toCv(pixels);
        convertColor(image, gray, CV_RGB2GRAY);
        Canny(gray, canny, cannyThreshold1*2.0, cannyThreshold2*2.0, 5);
        
        //  Hought
        //
//        vector<Vec4i> cvlines;
//        HoughLinesP(canny, cvlines, 1,(PI/180)*90,1,houghtMinLinLenght, houghtMaxLineGap);
//        lines.clear();
//        for( size_t i = 0; i < cvlines.size(); i++ ){
//            Line line;
//            line.a.set(cvlines[i][0],cvlines[i][1]);
//            line.b.set(cvlines[i][2],cvlines[i][3]);
//            lines.push_back(line);
//        }
//        
//        for(int i = 0; i < lines.size(); i++){
//            ofPoint vertexA = getVertex(lines[i].a);
//            ofPoint vertexB = getVertex(lines[i].b);
//            ofPoint diff = vertexA - vertexB;
//            
//            if(vertexA != ofPoint(0,0) && vertexB != ofPoint(0,0) && diff.length() < 20){
//                lineMesh.addVertex(vertexA+_center);
//                lineMesh.addVertex(vertexB+_center);
//            }
//        }
        
        //  Translate the images
        //
        ofPixels cannyOF;
        toOf(canny, cannyOF);
        vector<ofPolyline> lines = getPaths(cannyOF, minGapLength, minPathLength);
        
        for(int i = 0; i < lines.size(); i++){
            ofPoint prev = ofPoint(0,0);
            for(int j = 0; j < lines[i].size(); j++){
                ofPoint point = getVertex(lines[i][j]);
                ofPoint diff = point - prev;
                
                if(point != ofPoint(0,0) && prev != ofPoint(0,0) && diff.length() < 20){
                    lineMesh.addVertex(prev+_center);
                    lineMesh.addVertex(point+_center);
                }
                prev = point;
            }
        }
        bScrap = false;
    }
}

ofPoint SketchCity::getVertex(ofVec2f _pos){
    
    ofQuaternion ang_offset;
    ang_offset.makeRotate(180-sv.getDirection(), 0, 0, 1);
    float tiltAngle = sv.getTiltYaw()*DEG_TO_RAD;
    
    ofQuaternion tilt_offset;
    tilt_offset.makeRotate(sv.getTiltPitch(), cos(tiltAngle), sin(-tiltAngle), 0);
    
    float x = _pos.x/((float)sv.getWidth());
    float y = _pos.y/((float)sv.getHeight());
    
    float rad_azimuth = x * TWO_PI;
    float rad_elevation = y * PI;
    
    x *= ((float)sv.getDepthMapWidth());
    y *= ((float)sv.getDepthMapHeight());
    
    int index = ((int)y)*sv.getDepthMapWidth()+((int)x);
    int depthMapIndex = sv.depthmapIndices[index];
    
    if (depthMapIndex != 0){
        ofPoint pos;
        pos.x = sin(rad_elevation) * sin(rad_azimuth);
        pos.y = sin(rad_elevation) * cos(rad_azimuth);
        pos.z = cos(rad_elevation);
        
        ofPoint vertex;
        DepthMapPlane plane = sv.depthmapPlanes[depthMapIndex];
        double distance = -plane.d/(plane.x * pos.x + plane.y * pos.y + -plane.z * pos.z);
        
        vertex = ang_offset * tilt_offset * pos;
        vertex *= distance;
        vertex += ofPoint(0,0,sv.getGroundHeight());
        
        if(ofPoint(plane.x,plane.y,plane.z).dot(ofPoint(0,0,-1))<0.5 && distance < maxDistance){
            return vertex;
        }
    }
    
    return ofPoint(0,0);
}

typedef std::pair<int, int> intPair;
vector<ofPolyline> SketchCity::getPaths(ofPixels& img, float minGapLength, int minPathLength) {
	float minGapSquared = minGapLength * minGapLength;
	
	list<intPair> remaining;
	int w = img.getWidth(), h = img.getHeight();
	for(int y = 0; y < h; y++) {
		for(int x = 0; x < w; x++) {
			if(img.getColor(x, y).getBrightness() > 128) {
				remaining.push_back(intPair(x, y));
			}
		}
	}
	
	vector<ofPolyline> paths;
	if(!remaining.empty()) {
		int x = remaining.back().first, y = remaining.back().second;
		while(!remaining.empty()) {
			int nearDistance = 0;
			list<intPair>::iterator nearIt, it;
			for(it = remaining.begin(); it != remaining.end(); it++) {
				intPair& cur = *it;
				int xd = x - cur.first, yd = y - cur.second;
				int distance = xd * xd + yd * yd;
				if(it == remaining.begin() || distance < nearDistance) {
					nearIt = it, nearDistance = distance;
					// break for immediate neighbors
					if(nearDistance < 4) {
						break;
					}
				}
			}
			intPair& next = *nearIt;
			x = next.first, y = next.second;
			if(paths.empty()) {
				paths.push_back(ofPolyline());
			} else if(nearDistance >= minGapSquared) {
				if(paths.back().size() < minPathLength) {
					paths.back().clear();
				} else {
					paths.push_back(ofPolyline());
				}
			}
			paths.back().addVertex(ofVec2f(x, y));
			remaining.erase(nearIt);
		}
	}
	
	return paths;
}

void SketchCity::selfDraw(){
    materials["MATERIAL 1"]->begin();
    ofPushMatrix();
    ofTranslate(0,0,-25);
    ofRotate(90, 1, 0, 0);
    grid.draw();
    ofPopMatrix();
    
    ofPushStyle();
    ofSetColor(0,100);
    lineMesh.draw();
    ofPopStyle();
    
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
            ofTranslate(ofPoint(ofGetWidth()-sv.getWidth()-10,uiMap.getHeight()+20));
            ofPushMatrix();
            ofSetColor(255);
            drawMat(canny,0,0);
            ofSetColor(255,0,0);
            for(int i = 0; i < lines.size();i++){
                ofLine(lines[i].a, lines[i].b);
            }
            ofSetColor(255);
            
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
        lineMesh.clear();
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