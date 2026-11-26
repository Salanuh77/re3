#include "common.h"
#include "FixedRender.h"

#ifdef FIXED_TIME_STEP
#include <vector>
#include "FixedStep.h"
#include "Pools.h"
#include "main.h"
#include "Quaternion.h"
#include "Camera.h"
#include "PointLights.h"
#include "RwHelper.h"

namespace {
struct Snapshot
{
	int handle;
	RwObject *object;
	CFixedTransform previous, current;
	RwMatrix saved;
	CMatrix savedEntity;
	bool rendered;
	Snapshot(void) : handle(-1), object(nil), rendered(false) {}
};

std::vector<Snapshot> vehicles, peds, objects;
Snapshot camera;
bool haveCamera = false;
bool renderActive = false;
int16 simulationLightCount;
CRegisteredPointLight simulationLights[NUMPOINTLIGHTS];

CFixedTransform
ReadTransform(const RwMatrix &matrix)
{
	CFixedTransform transform;
	transform.position[0] = matrix.pos.x;
	transform.position[1] = matrix.pos.y;
	transform.position[2] = matrix.pos.z;
	RwMatrix normalized = matrix;
	CVector right(matrix.right), up(matrix.up), at(matrix.at);
	right.Normalise(); up.Normalise(); at.Normalise();
	normalized.right = right;
	normalized.up = up;
	normalized.at = at;
	CQuaternion rotation;
	rotation.Set(normalized);
	rotation.Normalise();
	transform.rotation[0] = rotation.x;
	transform.rotation[1] = rotation.y;
	transform.rotation[2] = rotation.z;
	transform.rotation[3] = rotation.w;
	return transform;
}

void
WriteTransform(RwFrame *frame, const CFixedTransform &transform)
{
	RwMatrix *matrix = RwFrameGetMatrix(frame);
	float rightScale = RwV3dLength(&matrix->right);
	float upScale = RwV3dLength(&matrix->up);
	float atScale = RwV3dLength(&matrix->at);
	CQuaternion rotation(transform.rotation[0], transform.rotation[1], transform.rotation[2], transform.rotation[3]);
	rotation.Get(matrix);
	RwV3dScale(&matrix->right, &matrix->right, rightScale);
	RwV3dScale(&matrix->up, &matrix->up, upScale);
	RwV3dScale(&matrix->at, &matrix->at, atScale);
	matrix->pos.x = transform.position[0];
	matrix->pos.y = transform.position[1];
	matrix->pos.z = transform.position[2];
	RwMatrixUpdate(matrix);
	RwFrameUpdateObjects(frame);
}

template<class Pool> void
Capture(Pool *pool, std::vector<Snapshot> &snapshots, bool before)
{
	if(!pool) return;
	snapshots.resize(pool->GetSize());
	for(int i = 0; i < pool->GetSize(); i++){
		auto entity = pool->GetSlot(i);
		Snapshot &snapshot = snapshots[i];
		if(!entity || !entity->m_rwObject){
			snapshot.handle = -1;
			continue;
		}
		RwMatrix matrix;
		matrix.right = entity->GetRight();
		matrix.up = entity->GetForward();
		matrix.at = entity->GetUp();
		matrix.pos = entity->GetPosition();
		CFixedTransform transform = ReadTransform(matrix);
		if(snapshot.handle != pool->GetIndex(entity) || snapshot.object != entity->m_rwObject){
			snapshot.handle = pool->GetIndex(entity);
			snapshot.object = entity->m_rwObject;
			snapshot.previous = snapshot.current = transform;
		}else if(before){
			snapshot.previous = transform;
		}else{
			snapshot.current = transform;
		}
	}
}

template<class Pool> void
Render(Pool *pool, std::vector<Snapshot> &snapshots, float alpha, bool restore)
{
	if(!pool) return;
	for(int i = 0; i < int(snapshots.size()) && i < pool->GetSize(); i++){
		auto entity = pool->GetSlot(i);
		Snapshot &snapshot = snapshots[i];
		if(!entity || snapshot.handle != pool->GetIndex(entity) || snapshot.object != entity->m_rwObject)
			continue;
		RwFrame *frame = (RwFrame*)rwObjectGetParent(entity->m_rwObject);
		if(!frame) continue;
		if(restore){
			if(snapshot.rendered){
				entity->GetMatrix().CopyOnlyMatrix(snapshot.savedEntity);
				*RwFrameGetMatrix(frame) = snapshot.saved;
				RwFrameUpdateObjects(frame);
#ifdef PED_SKIN
				if(entity->IsPed() && RwObjectGetType(entity->m_rwObject) == rpCLUMP && IsClumpSkinned(entity->GetClump()))
					entity->UpdateRpHAnim();
#endif
				snapshot.rendered = false;
			}
		}else{
			snapshot.saved = *RwFrameGetMatrix(frame);
			snapshot.savedEntity = entity->GetMatrix();
			WriteTransform(frame, CFixedTransform::Blend(snapshot.previous, snapshot.current, alpha));
			RwMatrix *matrix = RwFrameGetMatrix(frame);
			entity->GetRight() = matrix->right;
			entity->GetForward() = matrix->up;
			entity->GetUp() = matrix->at;
			entity->GetMatrix().GetPosition() = matrix->pos;
			snapshot.rendered = true;
		}
	}
}
}

void CFixedRender::Reset(void)
{
	End();
	vehicles.clear(); peds.clear(); objects.clear();
	haveCamera = false;
	simulationLightCount = 0;
}

void CFixedRender::BeforeStep(void)
{
	Capture(CPools::GetVehiclePool(), vehicles, true);
	Capture(CPools::GetPedPool(), peds, true);
	Capture(CPools::GetObjectPool(), objects, true);
	if(Scene.camera){
		camera.previous = ReadTransform(*RwFrameGetMatrix(RwCameraGetFrame(Scene.camera)));
		if(!haveCamera) camera.current = camera.previous;
		haveCamera = true;
	}
}

void CFixedRender::AfterStep(void)
{
	simulationLightCount = CPointLights::NumLights;
	memcpy(simulationLights, CPointLights::aLights, simulationLightCount * sizeof(simulationLights[0]));
	Capture(CPools::GetVehiclePool(), vehicles, false);
	Capture(CPools::GetPedPool(), peds, false);
	Capture(CPools::GetObjectPool(), objects, false);
	if(Scene.camera && haveCamera){
		camera.current = ReadTransform(*RwFrameGetMatrix(RwCameraGetFrame(Scene.camera)));
		if(TheCamera.Get_Just_Switched_Status())
			camera.previous = camera.current;
	}
}

void CFixedRender::RestoreLights(void)
{
	CPointLights::NumLights = simulationLightCount;
	memcpy(CPointLights::aLights, simulationLights, simulationLightCount * sizeof(simulationLights[0]));
}

void CFixedRender::Begin(float alpha)
{
	renderActive = true;
	Render(CPools::GetVehiclePool(), vehicles, alpha, false);
	Render(CPools::GetPedPool(), peds, alpha, false);
	Render(CPools::GetObjectPool(), objects, alpha, false);
	if(Scene.camera && haveCamera){
		RwFrame *frame = RwCameraGetFrame(Scene.camera);
		camera.saved = *RwFrameGetMatrix(frame);
		WriteTransform(frame, CFixedTransform::Blend(camera.previous, camera.current, alpha));
		camera.rendered = true;
	}
}

void CFixedRender::End(void)
{
	if(!renderActive) return;
	renderActive = false;
	Render(CPools::GetVehiclePool(), vehicles, 0.0f, true);
	Render(CPools::GetPedPool(), peds, 0.0f, true);
	Render(CPools::GetObjectPool(), objects, 0.0f, true);
	if(camera.rendered && Scene.camera){
		RwFrame *frame = RwCameraGetFrame(Scene.camera);
		*RwFrameGetMatrix(frame) = camera.saved;
		RwFrameUpdateObjects(frame);
		camera.rendered = false;
	}
}
#endif
