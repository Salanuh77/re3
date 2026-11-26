#pragma once

#include <cmath>
#include <cstdint>

// Keep the original game's 30 Hz handling while allowing arbitrary render rates.
class CFixedStep
{
	double m_accumulator;
public:
	static double StepSeconds(void) { return 1.0 / 30.0; }
	static unsigned MaxSteps(void) { return 5; }
	CFixedStep(void) : m_accumulator(0.0) {}
	void Reset(void) { m_accumulator = 0.0; }
	unsigned Advance(double elapsed, bool paused = false) {
		if(paused){
			Reset();
			return 0;
		}
		if(!std::isfinite(elapsed) || elapsed <= 0.0)
			return 0;
		// Bound both the input and the work. Drop overdue whole ticks, retain phase.
		m_accumulator += elapsed < 0.25 ? elapsed : 0.25;
		unsigned available = unsigned((m_accumulator + 1.0e-12) / StepSeconds());
		m_accumulator -= available * StepSeconds();
		if(m_accumulator < 0.0)
			m_accumulator = 0.0;
		return available < MaxSteps() ? available : MaxSteps();
	}
	float Alpha(void) const { return float(m_accumulator / StepSeconds()); }
};

class CFixedStepMilliseconds
{
	double m_fraction;
public:
	CFixedStepMilliseconds(void) : m_fraction(0.0) {}
	void Reset(void) { m_fraction = 0.0; }
	uint32_t Advance(void) {
		m_fraction += CFixedStep::StepSeconds() * 1000.0;
		uint32_t whole = uint32_t(m_fraction + 1.0e-9);
		m_fraction -= whole;
		return whole;
	}
};

// Independent of RenderWare so the exact render blend can be regression tested.
struct CFixedTransform
{
	float position[3];
	float rotation[4];

	static CFixedTransform Blend(const CFixedTransform &previous, const CFixedTransform &current, float alpha) {
		CFixedTransform result;
		alpha = alpha < 0.0f ? 0.0f : (alpha > 1.0f ? 1.0f : alpha);
		float distanceSquared = 0.0f;
		float dot = 0.0f;
		for(int i = 0; i < 3; i++){
			float delta = current.position[i] - previous.position[i];
			distanceSquared += delta * delta;
		}
		// Teleports/load transitions should snap, not sweep through the map.
		if(distanceSquared > 50.0f * 50.0f)
			return current;
		for(int i = 0; i < 4; i++)
			dot += previous.rotation[i] * current.rotation[i];
		float lengthSquared = 0.0f;
		for(int i = 0; i < 4; i++){
			float start = dot < 0.0f ? -previous.rotation[i] : previous.rotation[i];
			result.rotation[i] = start + (current.rotation[i] - start) * alpha;
			lengthSquared += result.rotation[i] * result.rotation[i];
		}
		if(lengthSquared > 0.0f){
			float inverseLength = 1.0f / std::sqrt(lengthSquared);
			for(int i = 0; i < 4; i++)
				result.rotation[i] *= inverseLength;
		}else{
			for(int i = 0; i < 4; i++)
				result.rotation[i] = current.rotation[i];
		}
		for(int i = 0; i < 3; i++)
			result.position[i] = previous.position[i] + (current.position[i] - previous.position[i]) * alpha;
		return result;
	}
};
