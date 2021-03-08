//
// Copyright (c) 2017-2020 the rbfx project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "../Precompiled.h"

#include "../Graphics/Drawable.h"
#include "../Graphics/Octree.h"
#include "../IO/Log.h"
#include "../RenderPipeline/CameraProcessor.h"
#include "../RenderPipeline/RenderPipelineDefs.h"
#include "../Scene/Node.h"

#include "../DebugNew.h"

namespace Urho3D
{

CameraProcessor::CameraProcessor(Context* context)
    : Object(context)
{
}

void CameraProcessor::SetCameras(ea::span<Camera* const> cameras)
{
    const unsigned numCameras = cameras.size();
    cameras_.resize(numCameras);
    ea::copy(cameras.begin(), cameras.end(), cameras_.begin());

    if (numCameras > 0)
    {
        isCameraFlippedByUser_ = cameras[0]->GetFlipVertical();
        for (unsigned i = 1; i < numCameras; ++i)
        {
            if (isCameraFlippedByUser_ != cameras[i]->GetFlipVertical())
            {
                URHO3D_LOGERROR("All Cameras used in RenderPipeline should have the same Flip Vertical flag value");
                assert(0);
            }
        }
    }
}

void CameraProcessor::UpdateCamera(const FrameInfo& frameInfo, Camera* camera)
{
    const Vector3 cameraPosition = camera->GetNode()->GetWorldPosition();
    Zone* cameraZone = frameInfo.octree_->QueryZone(cameraPosition, camera->GetZoneMask()).zone_;
    camera->SetZone(cameraZone);

    if (flipCameraForRendering_)
        camera->SetFlipVertical(!camera->GetFlipVertical());

    if (camera->GetAutoAspectRatio())
        camera->SetAspectRatioInternal(static_cast<float>(frameInfo.viewSize_.x_) / frameInfo.viewSize_.y_);
}

void CameraProcessor::OnUpdateBegin(const FrameInfo& frameInfo)
{
    flipCameraForRendering_ = false;

#ifdef URHO3D_OPENGL
    // On OpenGL, flip the projection if rendering to a texture so that the texture can be addressed in the same way
    // as a render texture produced on Direct3D
    if (frameInfo.renderTarget_)
        flipCameraForRendering_ = true;
#endif

    for (Camera* camera : cameras_)
    {
        if (camera)
            UpdateCamera(frameInfo, camera);
    }
}

void CameraProcessor::OnRenderEnd(const FrameInfo& frameInfo)
{
    for (Camera* camera : cameras_)
    {
        if (flipCameraForRendering_ && camera)
            camera->SetFlipVertical(!camera->GetFlipVertical());
    }
}

unsigned CameraProcessor::GetPipelineStateHash() const
{
    unsigned hash = 0;
    CombineHash(hash, isCameraFlippedByUser_);
    CombineHash(hash, flipCameraForRendering_);
    return hash;
}

}