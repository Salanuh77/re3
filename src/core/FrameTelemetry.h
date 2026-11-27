#pragma once

#include <cmath>

// Frame-start intervals measure rendering independently of simulation time scale.
// An interval belongs to the preceding frame, including its executed ticks.
class CFrameTelemetry
{
	double m_windowSeconds;
	unsigned m_windowFrames, m_windowTicks;
	bool m_started;
public:
	double frameSeconds, sampleSeconds;
	unsigned sampleFrames, sampleTicks, currentTicks;
	CFrameTelemetry(void) { Reset(); }
	void Reset(void) {
		m_windowSeconds = frameSeconds = sampleSeconds = 0;
		m_windowFrames = m_windowTicks = sampleFrames = sampleTicks = currentTicks = 0;
		m_started = false;
	}
	void BeginFrame(double seconds) {
		frameSeconds = std::isfinite(seconds) && seconds > 0 ? seconds : 0;
		if(m_started && frameSeconds > 0){
			m_windowSeconds += frameSeconds;
			m_windowFrames++;
			m_windowTicks += currentTicks;
			if(m_windowSeconds + 1.0e-12 >= 0.5){
				sampleSeconds = m_windowSeconds;
				sampleFrames = m_windowFrames;
				sampleTicks = m_windowTicks;
				m_windowSeconds = 0;
				m_windowFrames = m_windowTicks = 0;
			}
		}
		m_started = true;
		currentTicks = 0;
	}
	void RecordTick(void) { currentTicks++; }
	double FPS(void) const { return sampleSeconds > 0 ? sampleFrames / sampleSeconds : 0; }
	double PhysicsHz(void) const { return sampleSeconds > 0 ? sampleTicks / sampleSeconds : 0; }
	double AverageFrameMilliseconds(void) const { return sampleFrames ? 1000 * sampleSeconds / sampleFrames : 0; }
};
