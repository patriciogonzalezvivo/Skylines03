//
//  Prototype01.cpp
//
//  Created by Patricio Gonzalez Vivo on 9/12/13.
//
//

#include "Prototype01.h"

void Prototype01::selfSetup(){
    ofSetFullscreen(false);
    map.allocate(ofGetScreenWidth(), ofGetScreenHeight());
    map.loadMap("Bing");
    
    pathHeader.setUseTexture(false);
    
    ofRegisterURLNotification(this);
    nSelected = -1;
    nLoading = -1;
}

void Prototype01::selfSetupGuis(){
    backgroundSet(new UIMapBackground());
    guiAdd(map);
}

void Prototype01::selfSetupSystemGui(){
    sysGui->addLabel("From");
    origin = sysGui->addTextInput("From", "");
    origin->setAutoClear(false);
    
    sysGui->addLabel("To");
    destiny = sysGui->addTextInput("To", "");
    destiny->setAutoClear(false);
    
    sysGui->addToggle("Direction_Angle", &bDirAngle);
    
    sysGui->addButton("Save_Ids", &bSave);
}

void Prototype01::guiSystemEvent(ofxUIEventArgs &e){
    string name = e.widget->getName();
    
    if(name == "To" || name == "From"){
        string orig = origin->getTextString();
        for (int i = 0; i < orig.size(); i++) {
            if(orig[i] == ' '){
                orig[i] = '+';
            }
        }
        origin->setTextString(orig);
        
        string dest = destiny->getTextString();
        for (int i = 0; i < dest.size(); i++) {
            if(dest[i] == ' '){
                dest[i] = '+';
            }
        }
        destiny->setTextString(dest);
        
        string data_url = "https://maps.googleapis.com/maps/api/directions/xml?origin="+orig+"&destination="+dest+"&sensor=false&mode=walking&key=AIzaSyCJgZNH4orMJ0gLR04CB0XC0igj0fbGTxo";
        
        ofLoadURLAsync(data_url);
        directionsLoaded = false;
    } else if(name == "Save_Ids"){
        if(bSave){
            ofBuffer buffer;
            for(int i = 0; i < path.size(); i++){
                buffer.append(path[i].pano_id+"\n");
            }
            ofBufferToFile(ofGetTimestampString()+".txt", buffer);
        }
    }
}

void Prototype01::selfSetupRenderGui(){

}

void Prototype01::guiRenderEvent(ofxUIEventArgs &e){
    string name = e.widget->getName();
}

void Prototype01::urlResponse(ofHttpResponse &response){

    if(response.status==200 && response.request.url.find("https://maps.googleapis.com/maps/api/directions/xml") == 0){
        directions.clear();
        directionsLine.clear();
        pathHeader.clear();
        path.clear();
        
        ofxXmlSettings  XML;
        string data_prop_pano_id;
        XML.loadFromBuffer(response.data);
        {
            XML.pushTag("DirectionsResponse");
            XML.pushTag("route");
            XML.pushTag("leg");
            int steps = XML.getNumTags("step");
            
            for (int i = 0; i < steps; i++) {
                if(XML.pushTag("step",i)){
                    
                    if(i == 0){
                        Location loc;
                        loc.lat = XML.getValue("start_location:lat", 0.0);
                        loc.lon = XML.getValue("start_location:lng", 0.0);
                        directions.push_back(loc);
                    }
                    
                    Location loc;
                    loc.lat = XML.getValue("end_location:lat", 0.0);
                    loc.lon = XML.getValue("end_location:lng", 0.0);
                    directions.push_back(loc);
                    
                    XML.popTag();
                }
            }
            
            directionsLoaded = true;
        }
    }
}

void Prototype01::selfUpdate(){
    map.update();
    
    if(directionsLoaded){
        if(directions.size()>1){
            nLoading = 0;
            pathHeader.setLocation(directions[nLoading]);
            directionsLoaded = false;
        }
    }
    
    if(nSelected!=-1){
        if(ofGetKeyPressed('c')){
            float angle = path[nSelected].loc.getDegTo(mouseLocation);
            int closer = getCloser(path[nSelected].links, angle*DEG_TO_RAD);
            changePathAt(nSelected, closer);
            nSelected = -1;
        }
    } else if(nLoading!=-1 && !bSave && !bEdit){
        int last = path.size()-1;
        
        if(pathHeader.links.size()!=0){
            
            //  Get Actual Location
            //
            Location headerLoc = pathHeader.getLocation();
            
            //  Distance (in meters) to complete the next direction
            //
            float dist = headerLoc.getDistanceTo(directions[nLoading+1]);
//            cout << "Distance to Rule #" << nLoading+1 << " is " << dist << "m" << endl;
            if(dist < 10){
                nLoading++;
                
                if(nLoading==directions.size()-1){
                    nLoading = -1;
                    
//                    cout << "Adding last step: " << pathHeader.getPanoId() << endl;
                    path.push_back(pathHeader);
                    return;
                }
            }
            
            //  Chose the one in the right direction
            //
            int closer = -1;
            if(pathHeader.links.size()==2&&path.size()>0){
                // Follow the road
                //
                string prev = path[path.size()-1].pano_id;
                closer = ((pathHeader.links[0].pano_id!=prev)?0:1);
            } else {
                
                ofPoint diff = directionsLine[nLoading]-directionsLine[nLoading+1];
                float angle = atan2(diff.y, diff.x)-PI;
                angle *=RAD_TO_DEG;
                angle = angle+90;
                if(angle > 0){
                    angle = 360 - angle;
                } else {
                    angle = abs(angle);
                }
                
                float angle2 = headerLoc.getDegTo(directions[nLoading+1]);
                
                closer = getCloser(pathHeader.links, (bDirAngle?angle:angle2)*DEG_TO_RAD);
                
                if(path.size()>0 && closer != -1){
                    string prev = path[path.size()-2].pano_id;
                    if(pathHeader.links[closer].pano_id == prev){
                        //  Is comming back
                        //
                        closer = -1;
                    }
                }
            }
            
            if(closer != -1){
//                cout << "Adding step: " << pathHeader.getPanoId() << endl;
                path.push_back(pathHeader);
                path[path.size()-1].nDirection = nLoading;
                
                //  Request the next one
                //
                pathHeader.setPanoId(pathHeader.links[closer].pano_id);
            } else {
                
                //  Maybe he skip steps
                //
//                for (int i = nLoading+1; i<directions.size(); i++) {
//                    float dist = headerLoc.getDistanceTo(directions[i]);
//                    if(dist < 10){
//                        cout << "Skiping steps to " << i << endl;
//                        nLoading = i;
//                    }
//                }
            }
        }
    }
    
    //  Update positions over map
    //
    directionsLine.clear();
    for (int i = 0; i < directions.size(); i++) {
        directionsLine.addVertex(map.locationPoint(directions[i]));
    }
    
    pathLine.clear();
    for (int i = 0; i < path.size(); i++) {
        pathLine.addVertex(map.locationPoint(path[i].loc));
    }
}

int  Prototype01::getCloser(const vector<Link> &_links, float _angle){
    ofPoint direction = ofPoint(cos(_angle),sin(_angle));
    
    int closer = -1;
    float minDiff = 0;
    for (int i = 0; i < _links.size(); i++) {
        
        float angle = _links[i].yaw_deg*DEG_TO_RAD;
        ofPoint l = ofPoint(cos(angle),sin(angle));
        
        float diff = l.dot(direction);
        if( diff >= 0 && diff > minDiff){
            minDiff = diff;
            closer = i;
        }
    }
    
    return closer;
}

void Prototype01::changePathAt(int _nIndex, int _link){
    
    // Restore the direction number
    //
    nLoading = path[_nIndex].nDirection;
    
    //  Erase following steps
    //
    int firstTime = -1;
    for (int i = 0; i < path.size(); i++) {
        if(path[i].pano_id == path[_nIndex].pano_id){
            firstTime = i;
            break;
        }
    }

    if (firstTime==-1) {
        cout << "ERROR" << endl;
        return;
    }
    
    path.erase(path.begin()+firstTime+1,path.end());
    //  Request the choosen direction
    //
    pathHeader.setPanoId(path[_nIndex].links[_link].pano_id);
}

void Prototype01::selfDraw(){
    ofSetColor(255);
    map.draw();
    
    ofPushStyle();
    ofSetLineWidth(2);
    ofSetColor(255, 0, 0);
    directionsLine.draw();
    for(int i = 0; i < directionsLine.size(); i++){
        ofCircle(directionsLine[i], 3);
    }
    ofSetColor(0, 255, 0);
    pathLine.draw();
    for(int i = 0; i < pathLine.size(); i++){
        ofCircle(pathLine[i], 3);
    }
    ofPopStyle();
    
    if(nSelected!=-1){
        ofPushStyle();
        ofPushMatrix();
        
        ofSetColor(255, 0, 255);
        ofNoFill();
        ofCircle(0,0,5);
        ofSetLineWidth(2);
        
        ofTranslate(pathLine[nSelected]);
        
        float angle = path[nSelected].loc.getDegTo(mouseLocation);
        int closer = getCloser(path[nSelected].links, angle*DEG_TO_RAD);
        
        ofPoint mouse = ofPoint(ofGetMouseX(),ofGetMouseY())-pathLine[nSelected];
        ofLine(ofPoint(),mouse);
        ofDrawBitmapStringHighlight(ofToString(angle), mouse);
        
        for (int i = 0; i < path[nSelected].links.size(); i++) {
            float angle = ofDegToRad(path[nSelected].links[i].yaw_deg)-PI*0.5;
            float radius = 100;
            
            ofPoint pos = ofPoint(cos(angle),sin(angle))*radius;
            ofLine(ofPoint(0,0),pos);
            
            ofColor bg = ofColor(0);
            ofColor fg = ofColor(255);
            
            if(i==closer){
                fg = ofColor(255,0,255);
            }
            
            ofDrawBitmapStringHighlight(ofToString(path[nSelected].links[i].yaw_deg), pos,bg,fg);
        }
    
        ofPopMatrix();
        ofPopStyle();
    }

    if(bDebug){
        ofDrawBitmapStringHighlight("Lat: "+ofToString(mouseLocation.lat)+'\n'+
                                    "Lon: "+ofToString(mouseLocation.lon), ofGetMouseX(),ofGetMouseY());
    }
}

void Prototype01::selfEnd(){
    ofUnregisterURLNotification(this);
}

void Prototype01::selfKeyPressed(ofKeyEventArgs & args){
    if (bEdit) {
        // Restore the direction number
        //
        if(args.key == 'd'){
            nLoading = path[nSelected].nDirection;
            for (int i = path.size()-1; i >= 0; i--) {
                if(path[i].pano_id == path[nSelected].pano_id){
                    path.erase(path.begin()+i);
                }
            }
            nSelected = -1;
        }
        
    } else {
        if (args.key == '<' || args.key == ',' || args.key == '=' || args.key == '+' ) {
            if (map.getZoom() < 19) {
                map.setZoom(map.getZoom()+1);
            }
        } else if (args.key == '>' || args.key == '.'|| args.key == '-' || args.key == '_') {
            if (map.getZoom() > 0) {
                map.setZoom(map.getZoom()-1);
            }
        } else if (args.key == OF_KEY_UP ) {
            map.setCenter(map.getCenterCoordinate().up());
        } else if (args.key == OF_KEY_DOWN ) {
            map.setCenter(map.getCenterCoordinate().down());
        } else if (args.key == OF_KEY_LEFT ) {
            map.setCenter(map.getCenterCoordinate().left());
        } else if (args.key == OF_KEY_RIGHT ) {
            map.setCenter(map.getCenterCoordinate().right());
        }
    }
}

void Prototype01::selfKeyReleased(ofKeyEventArgs & args){
}

void Prototype01::selfMouseMoved(ofMouseEventArgs& data){
    mouseLocation = map.pointLocation(ofPoint(data.x,data.y));
}

void Prototype01::selfMousePressed(ofMouseEventArgs& data){
    
    if(ofGetKeyPressed('c') && nSelected != -1){

    } else {
        nSelected = -1;
        
        for(int i = 0; i < pathLine.size(); i++){
            if(pathLine[i].distance(ofPoint(data.x,data.y)) <= 10){
                nSelected = i;
                break;
            }
        }
        
        if( nSelected != -1){
            mouseLocation = map.pointLocation(pathLine[nSelected]);
        } else if(ofGetKeyPressed('a')){
            Location loc = map.pointLocation(ofPoint(data.x,data.y));
            string org = ofToString(loc.lat)+","+ofToString(loc.lon);
            origin->setTextString(org);
            
            string data_url = "https://maps.googleapis.com/maps/api/directions/xml?origin="+origin->getTextString()+"&destination="+destiny->getTextString()+"&sensor=false&mode=walking&key=AIzaSyCJgZNH4orMJ0gLR04CB0XC0igj0fbGTxo";
            ofLoadURLAsync(data_url);
            directionsLoaded = false;
            
        } else if(ofGetKeyPressed('z')){
            Location loc = map.pointLocation(ofPoint(data.x,data.y));
            string org = ofToString(loc.lat)+","+ofToString(loc.lon);
            destiny->setTextString(org);
            
            string data_url = "https://maps.googleapis.com/maps/api/directions/xml?origin="+origin->getTextString()+"&destination="+destiny->getTextString()+"&sensor=false&mode=walking&key=AIzaSyCJgZNH4orMJ0gLR04CB0XC0igj0fbGTxo";
            ofLoadURLAsync(data_url);
            directionsLoaded = false;
            
        }
    }
}

void Prototype01::selfMouseDragged(ofMouseEventArgs& data){
    ofPoint mouse = ofPoint(data.x,data.y);
    ofPoint pMouse = ofPoint(ofGetPreviousMouseX(),ofGetPreviousMouseY());

    float scale = pow(2.0, map.getZoom());
    ofPoint dMouse = (mouse - pMouse);
    
    if (data.button == 0) {
        map.panBy(dMouse);
    } else if (data.button == 2) {
        if (ofGetKeyPressed( OF_KEY_SHIFT )) {
            map.rotateBy(dMouse.x < 0 ? M_PI/72.0 : -M_PI/72.0, mouse);
        } else {
            map.scaleBy(dMouse.y < 0 ? 1.05 : 1.0/1.05, mouse);
        }
    }
    map.setCenter(map.getCenter());
}

void Prototype01::selfMouseReleased(ofMouseEventArgs& data){
}

void Prototype01::selfWindowResized(ofResizeEventArgs & args){
    map.allocate(ofGetWidth(), ofGetHeight());
}