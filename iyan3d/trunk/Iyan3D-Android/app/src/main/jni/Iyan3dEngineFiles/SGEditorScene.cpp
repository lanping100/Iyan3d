//
//  SGEditorScene.cpp
//  Iyan3D
//
//  Created by Karthik on 22/12/15.
//  Copyright © 2015 Smackall Games. All rights reserved.
//

#include "HeaderFiles/SGEditorScene.h"
#include "HeaderFiles/SGCloudRenderingHelper.h"

string constants::BundlePath = " ";
string constants::CachesStoragePath = " ";
string constants::DocumentsStoragePath="";
string constants::impotedImagesPathAndroid="";
float constants::iOS_Version = 0;

SGEditorScene::SGEditorScene(DEVICE_TYPE device,SceneManager *smgr,int screenWidth,int screenHeight)
{
    SceneHelper::screenWidth = screenWidth;
    SceneHelper::screenHeight = screenHeight;
    this->smgr = smgr;
    viewCamera =  SceneHelper::initViewCamera(smgr, cameraTarget, cameraRadius);
    
    initVariables(smgr, device);

#ifndef UBUNTU
    initTextures();
    jointSphereMesh = CSGRMeshFileLoader::createSGMMesh(constants::BundlePath + "/sphere.sgm",smgr->device);
    rotationCircle = SceneHelper::createCircle(smgr);
    sceneControls.clear();
    sceneControls = SceneHelper::initControls(smgr);
    BoneLimitsHelper::init();
    AutoRigJointsDataHelper::getTPoseJointsData(tPoseJoints);
#endif

    renderCamera = SceneHelper::initRenderCamera(smgr, cameraFOV);
}

SGEditorScene::~SGEditorScene()
{
    ShaderManager::lightPosition.clear();
    ShaderManager::lightColor.clear();
    ShaderManager::lightFadeDistances.clear();
    
    for(int i = 0;i < nodes.size();i++){
        if(nodes[i])
            delete nodes[i];
    }
    nodes.clear();   
    
    if (renderCamera)
        renderCamera.reset();
    if(viewCamera)
        viewCamera.reset();
    if(lightCamera)
        lightCamera.reset();
    
    if(ikJointsPositionMap.size())
        ikJointsPositionMap.clear();
    
    nodes.clear();
    jointSpheres.clear();
    tPoseJoints.clear();
    
    if(renderingTextureMap.size())
        renderingTextureMap.clear();
    if(sceneControls.size())
        sceneControls.clear();
    
    if(smgr)
        delete smgr;
    if(shaderMGR)
        delete shaderMGR;
    if(renHelper)
        delete renHelper;
    if(selectMan)
        delete selectMan;
    if(updater)
        delete updater;
    if(loader)
        delete loader;
    if(moveMan)
        delete moveMan;
    if(actionMan)
        delete actionMan;
    if(writer)
        delete writer;
    if(animMan)
        delete animMan;
    if(jointSphereMesh)
        delete jointSphereMesh;
    if(rotationCircle)
        delete rotationCircle;
    if(cmgr)
        delete cmgr;
    if(objLoader)
        delete objLoader;
}
void SGEditorScene::enterOrExitAutoRigMode(bool rigMode)
{
    isRigMode = rigMode;
    if(rigMode){
        rigMan = new SGAutoRigSceneManager(smgr,this);
        //setTransparencyForObjects();
        selectMan->unselectObject(selectedNodeId);
        clearSelections();
    }
    else{
        delete rigMan;
        updater->resetMaterialTypes(false);
    }
}

void SGEditorScene::initVariables(SceneManager* sceneMngr, DEVICE_TYPE devType)
{
    cmgr = new CollisionManager();
    shaderMGR = new ShaderManager(sceneMngr, devType);
    renHelper = new RenderHelper(sceneMngr, this);
    selectMan = new SGSelectionManager(sceneMngr, this);
    updater = new SGSceneUpdater(sceneMngr, this);
    loader = new SGSceneLoader(sceneMngr, this);
    moveMan = new SGMovementManager(sceneMngr, this);
    actionMan = new SGActionManager(sceneMngr, this);
    writer = new SGSceneWriter(sceneMngr, this);
    animMan = new SGAnimationManager(sceneMngr, this);
    objMan = new SGOBJManager(sceneMngr, this);

    isMultipleSelection = false;
    isJointSelected = isNodeSelected = isControlSelected = false;
    freezeRendering = isPlaying = isPreviewMode = isRigMode = false;
    selectedNodeId = selectedJointId = NOT_EXISTS;
    selectedNode = NULL;
    selectedJoint = NULL;
    jointSpheres.clear();
    tPoseJoints.clear();
    controlType = MOVE;
    selectedControlId = NOT_EXISTS;
    selectedNode = NULL;
    controlsPlane = new Plane3D();
    objLoader = new OBJMeshFileLoader;
    nodes.clear();
    totalFrames = 24;
    currentFrame = previousFrame = 0;
    cameraFOV = 72.0;
    cameraResolutionType = 0;
    
    camPreviewOrigin.x = 0.0;
    camPreviewOrigin.y = SceneHelper::screenHeight - (SceneHelper::screenHeight * CAM_PREV_PERCENT);
    float camPrevRatio = RESOLUTION[cameraResolutionType][1] / ((SceneHelper::screenHeight) * CAM_PREV_PERCENT);
    camPreviewEnd.x = camPreviewOrigin.x + RESOLUTION[cameraResolutionType][0] / camPrevRatio;
    camPreviewEnd.y = camPreviewOrigin.y + RESOLUTION[cameraResolutionType][1] / camPrevRatio;
    camPreviewScale = 1.0;
    previousDistance = Vector2(0.0, 0.0);
    actionObjectsSize = 0;
    moveNodeId = NOT_SELECTED;
    moveJointId = NOT_SELECTED;
}

void SGEditorScene::initTextures()
{
    bgTexture = smgr->loadTexture("bgtex",constants::BundlePath +  "/bgImageforall.png",TEXTURE_RGBA8,TEXTURE_BYTE);
    touchTexture = smgr->createRenderTargetTexture("TouchTexture", TEXTURE_RGBA8, TEXTURE_BYTE, TOUCH_TEXTURE_WIDTH, TOUCH_TEXTURE_HEIGHT);
    watermarkTexture = smgr->loadTexture("waterMarkTexture",constants::BundlePath + "/watermark.png",TEXTURE_RGBA8,TEXTURE_BYTE);

    previewTexture = smgr->createRenderTargetTexture("previewTexture", TEXTURE_RGBA8, TEXTURE_BYTE, PREVIEW_TEXTURE_WIDTH, PREVIEW_TEXTURE_HEIGHT);
    thumbnailTexture = smgr->createRenderTargetTexture("thumbnailTexture", TEXTURE_RGBA8, TEXTURE_BYTE, THUMBNAIL_TEXTURE_WIDTH, THUMBNAIL_TEXTURE_HEIGHT);
    shadowTexture = smgr->createRenderTargetTexture("shadowTexture", TEXTURE_DEPTH32, TEXTURE_BYTE, SHADOW_TEXTURE_WIDTH, SHADOW_TEXTURE_HEIGHT);
    whiteBorderTexture = smgr->loadTexture("whiteborder",constants::BundlePath + "/whiteborder.png",TEXTURE_RGBA8,TEXTURE_BYTE);
    
    renderingTextureMap[RESOLUTION[0][0]] = smgr->createRenderTargetTexture("RenderTexture", TEXTURE_RGBA8, TEXTURE_BYTE,RESOLUTION[0][0] , RESOLUTION[0][1]);
    renderingTextureMap[RESOLUTION[1][0]] = smgr->createRenderTargetTexture("RenderTexture", TEXTURE_RGBA8, TEXTURE_BYTE,RESOLUTION[1][0] , RESOLUTION[1][1]);
    renderingTextureMap[RESOLUTION[2][0]] = smgr->createRenderTargetTexture("RenderTexture", TEXTURE_RGBA8, TEXTURE_BYTE,RESOLUTION[2][0] , RESOLUTION[2][1]);

}

void SGEditorScene::renderAll()
{
    if(freezeRendering)
        return;
    
    bool displayPrepared = smgr->PrepareDisplay(SceneHelper::screenWidth, SceneHelper::screenHeight, true, true, false,
                                                Vector4(0, 0, 0, 255));
    
    if(displayPrepared) {
        rotationCircle->node->setVisible(false);
        smgr->draw2DImage(bgTexture, Vector2(0, 0), Vector2(SceneHelper::screenWidth,  SceneHelper::screenHeight), true,
                          smgr->getMaterialByIndex(SHADER_DRAW_2D_IMAGE));
        
        smgr->Render();
        
        renHelper->drawGrid();
        renHelper->drawCircle();
        renHelper->drawMoveAxisLine();
        renHelper->renderControls();
        //        // rtt division atlast post and pre stage
        renHelper->postRTTDrawCall();
        renHelper->rttDrawCall();
        smgr->EndDisplay(); // draws all the rendering command
        
        moveMan->swipeToRotate();
        if(!isRigMode)
            setTransparencyForObjects();
    }
}

void SGEditorScene::setTransparencyForObjects()
{
    if(!smgr && nodes.size() < 3)
        return;
    
    if((nodes[nodes.size()-1]->isTempNode && !isPreviewMode) || isRigMode) {
        isPreviewMode = true;
        int excludeNodeId = (isRigMode) ? selectedNodeId : nodes.size()-1;
        for(int index = 0; index < nodes.size(); index++) {
            if(index != excludeNodeId){
                nodes[index]->props.transparency = 0.2;
            }
        }
    } else if(!nodes[nodes.size()-1]->isTempNode && isPreviewMode) {
        isPreviewMode = false;
        for(int index = 0; index < nodes.size(); index++) {
            if(nodes[index]->props.isVisible)
                nodes[index]->props.transparency = 1.0;
        }
    }
}

Vector4 SGEditorScene::getCameraPreviewLayout()
{
    float camPrevRatio = RESOLUTION[cameraResolutionType][1] / ((SceneHelper::screenHeight) * CAM_PREV_PERCENT * camPreviewScale);
    float limitX = topRight.x - (RESOLUTION[cameraResolutionType][0] / camPrevRatio);
    float limitY = SceneHelper::screenHeight - (RESOLUTION[cameraResolutionType][1] / camPrevRatio);
    
    if(previousDistance != renHelper->cameraPreviewMoveDist && camPreviewOrigin.x >= topLeft.x && camPreviewOrigin.y >= topLeft.y && camPreviewOrigin.x <= limitX && camPreviewOrigin.y <= limitY) {
        camPreviewOrigin.x -= renHelper->cameraPreviewMoveDist.x;
        camPreviewOrigin.y -= renHelper->cameraPreviewMoveDist.y;
    } else {
        camPreviewOrigin.x = (camPreviewOrigin.x < topLeft.x) ? topLeft.x : camPreviewOrigin.x;
        camPreviewOrigin.y = (camPreviewOrigin.y < topLeft.y) ? topLeft.y : camPreviewOrigin.y;
        camPreviewOrigin.x = (camPreviewOrigin.x > limitX) ? limitX : camPreviewOrigin.x;
        camPreviewOrigin.y = (camPreviewOrigin.y > limitY) ? limitY : camPreviewOrigin.y;
    }
    camPreviewEnd.x = camPreviewOrigin.x + RESOLUTION[cameraResolutionType][0] / camPrevRatio;
    camPreviewEnd.y = camPreviewOrigin.y + RESOLUTION[cameraResolutionType][1] / camPrevRatio;
    previousDistance = renHelper->cameraPreviewMoveDist;
    return Vector4(camPreviewOrigin.x, camPreviewOrigin.y, camPreviewEnd.x, camPreviewEnd.y);
}

void SGEditorScene::findAndInsertInIKPositionMap(int jointId)
{
    shared_ptr<AnimatedMeshNode> rigNode = dynamic_pointer_cast<AnimatedMeshNode>(selectedNode->node);
    if(ikJointsPositionMap.find(jointId) == ikJointsPositionMap.end())
        ikJointsPositionMap.insert(pair<int,Vector3>(jointId,rigNode->getJointNode(jointId)->getAbsolutePosition()));
    else
        ikJointsPositionMap.find(jointId)->second = rigNode->getJointNode(jointId)->getAbsolutePosition();
}

void SGEditorScene::getIKJointPosition()
{
    shared_ptr<AnimatedMeshNode> rigNode = dynamic_pointer_cast<AnimatedMeshNode>(selectedNode->node);
    if(rigNode) {
        findAndInsertInIKPositionMap(LEG_RIGHT);
        findAndInsertInIKPositionMap(LEG_LEFT);
        findAndInsertInIKPositionMap(HAND_RIGHT);
        findAndInsertInIKPositionMap(HAND_LEFT);
    }
}


MIRROR_SWITCH_STATE SGEditorScene::getMirrorState()
{
    return actionMan->getMirrorState();
}

void SGEditorScene::initLightCamera(Vector3 position)
{
    lightCamera = smgr->createCameraNode("LightUniforms");
    lightCamera->setFOVInRadians(150.0 * DEGTORAD);
    lightCamera->setNearValue(5.0);
    updater->updateLightCam(position);
}

void SGEditorScene::clearSelections()
{
    isNodeSelected = isJointSelected = false;
    selectedNode = NULL;
    selectedJoint = NULL;
    selectedNodeId = selectedJointId = NOT_SELECTED;
}

void SGEditorScene::shaderCallBackForNode(int nodeID,string matName)
{
    for(int i = 0; i < nodes.size();i++){
        if(nodes[i]->node->getID() == nodeID){
            shaderMGR->setUniforms(nodes[i],matName);
            break;
        }
    }
}
bool SGEditorScene::isNodeTransparent(int nodeId)
{
    if(nodeId == -1)
        return false;
    else{
        for(int i = 0; i < nodes.size();i++){
            if(nodes[i]->node->getID() == nodeId){
                return (nodes[i]->props.transparency < 1.0) || nodes[i]->props.isSelected || (!nodes[i]->props.isVisible);
                break;
            }
        }
    }
}

void SGEditorScene::setJointsUniforms(int nodeID,string matName)
{
    shaderMGR->setUniforms(jointSpheres[nodeID - JOINT_SPHERES_START_ID],matName);
}
void SGEditorScene::setRotationCircleUniforms(int nodeID,string matName)
{
    shaderMGR->setUniforms(rotationCircle,matName);
}
bool SGEditorScene::isJointTransparent(int nodeID,string matName)
{
    return (jointSpheres[nodeID - JOINT_SPHERES_START_ID]->props.transparency < 1.0);
}
void SGEditorScene::setControlsUniforms(int nodeID,string matName)
{
    shaderMGR->setUniforms(sceneControls[nodeID - CONTROLS_START_ID],matName);
}
bool SGEditorScene::isControlsTransparent(int nodeID,string matName)
{
    return (sceneControls[nodeID - CONTROLS_START_ID]->props.transparency < 1.0);
}

bool SGEditorScene::hasNodeSelected()
{
    return (isRigMode && rigMan) ? rigMan->isNodeSelected : isNodeSelected;
}
bool SGEditorScene::hasJointSelected()
{
    return (isRigMode && rigMan) ? rigMan->isSGRJointSelected : isJointSelected;
}
SGNode* SGEditorScene::getSelectedNode()
{
    if(isRigMode && rigMan->isNodeSelected)
        return rigMan->selectedNode;
    else if (isNodeSelected)
        return selectedNode;
    return NULL;
}
SGJoint* SGEditorScene::getSelectedJoint()
{
    if(isRigMode && rigMan->isSGRJointSelected)
        return rigMan->selectedJoint;
    else if (isJointSelected)
        return selectedJoint;
    return NULL;    
}

shared_ptr<Node> SGEditorScene::getParentNode()
{
    return selectMan->getParentNode();
}

bool SGEditorScene::loadSceneData(std::string *filePath)
{
    return loader->loadSceneData(filePath);
}

void SGEditorScene::saveSceneData(std::string *filePath)
{
    writer->saveSceneData(filePath);
}

bool SGEditorScene::checkNodeSize(){
    if(nodes.size() < NODE_LIGHT + 1)
        return false;
    return true;
}

void SGEditorScene::saveThumbnail(char* targetPath)
{
    if(!checkNodeSize())
        return;
    
    bool displayPrepared = smgr->PrepareDisplay(thumbnailTexture->width,thumbnailTexture->height,false,true,false,Vector4(255,255,255,255));
    if(!displayPrepared)
        return;
    
    selectMan->unselectObjects();
    renHelper->setControlsVisibility(false);
    renHelper->setJointSpheresVisibility(false);
    rotationCircle->node->setVisible(false);
    //jointSpheres.clear();
    for(unsigned long i = NODE_CAMERA; i < nodes.size(); i++) {
        if(!(nodes[i]->props.isVisible))
            nodes[i]->node->setVisible(false);
    }
    
   selectMan->unselectObject(selectedNodeId);
    
    smgr->setRenderTarget(thumbnailTexture,true,true,false,Vector4(255,255,255,255));
    smgr->draw2DImage(bgTexture,Vector2(0,0),Vector2(SceneHelper::screenWidth,SceneHelper::screenHeight),true,smgr->getMaterialByIndex(SHADER_DRAW_2D_IMAGE));
    renHelper->drawGrid();
    
    smgr->Render();
    smgr->EndDisplay();
    smgr->writeImageToFile(thumbnailTexture, targetPath , (shaderMGR->deviceType == OPENGLES2) ? FLIP_VERTICAL : NO_FLIP);
    
    smgr->setRenderTarget(NULL,true,true,false,Vector4(255,255,255,255));
    
    for(unsigned long i = NODE_CAMERA; i < nodes.size(); i++) {
        if(!(nodes[i]->props.isVisible))
            nodes[i]->node->setVisible(true);
    }
    
    if(selectedNodeId != NOT_SELECTED)
        nodes[selectedNodeId]->props.isSelected = true;
    
    selectMan->selectObject(selectedNodeId);
    renHelper->setControlsVisibility(true);
    smgr->EndDisplay();
}

int SGEditorScene::undo(int &returnValue2)
{
    return actionMan->undo(returnValue2);
}
int SGEditorScene::redo()
{
    return actionMan->redo();
}

bool SGEditorScene::generateSGFDFile(int frame)
{
	updater->setDataForFrame(frame);
	return SGCloudRenderingHelper::writeFrameData(this, smgr, frame);
}

void SGEditorScene::setLightingOn()
{
    ShaderManager::sceneLighting = true;
}

void SGEditorScene::setLightingOff()
{
    ShaderManager::sceneLighting = false;
}

void SGEditorScene::popLightProps()
{
    if(ShaderManager::lightPosition.size() > 1) {
        ShaderManager::lightPosition.pop_back();
        ShaderManager::lightColor.pop_back();
        ShaderManager::lightFadeDistances.pop_back();
    }
    
}

void SGEditorScene::clearLightProps()
{
    while(ShaderManager::lightPosition.size() > 1)
        popLightProps();
}

bool SGEditorScene::isNodeInSelection(SGNode *sgNode)
{
    bool status = false;
    for(int i = 0; i < selectedNodeIds.size(); i++) {
        if(nodes[selectedNodeIds[i]] == sgNode) {
            status = true;
            break;
        }
    }
    return status;
}

bool SGEditorScene::allObjectsScalable()
{
    bool status = true;
    for(int i = 0; i < selectedNodeIds.size(); i++) {
        NODE_TYPE nType = nodes[selectedNodeIds[i]]->getType();
        if(nType == NODE_CAMERA || nType == NODE_LIGHT || nType == NODE_ADDITIONAL_LIGHT) {
            status = false;
            break;
        }
    }
    return status;
}

bool SGEditorScene::allNodesRemovable()
{
    bool status = true;
    for(int i = 0; i < selectedNodeIds.size(); i++) {
        NODE_TYPE nType = nodes[selectedNodeIds[i]]->getType();
        if(nType == NODE_CAMERA || nType == NODE_LIGHT) {
            status = false;
            break;
        }
    }
    return status;
}

Vector3 SGEditorScene::getSelectedNodeScale()
{
    if(selectedNodeIds.size() > 0)
        return getParentNode()->getScale();
    else if(isNodeSelected) {
        if(isJointSelected && selectedNode->getType() == NODE_TEXT)
            return selectedNode->joints[selectedJointId]->jointNode->getScale();
        else
            return selectedNode->node->getScale();
    }
    
    return Vector3(1.0);
}

Vector3 SGEditorScene::getPivotPoint(bool initial)
{
    if(getParentNode() && initial)
        return getParentNode()->getBoundingBox().getCenter();
    else if (getParentNode())
        return getParentNode()->getAbsolutePosition();
    return Vector3(0.0);
}
