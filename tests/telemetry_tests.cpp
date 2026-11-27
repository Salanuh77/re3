#include "FixedStep.h"
#include "FrameTelemetry.h"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>

static unsigned checks;
static bool Near(double a, double b) { return std::fabs(a - b) < 1.0e-6; }
static void Check(bool value, const char *message) {
	checks++;
	if(!value) throw std::runtime_error(message);
}
static CFrameTelemetry Simulate(unsigned fps, double scale = 1, bool paused = false) {
	CFixedStep clock;
	CFrameTelemetry stats;
	stats.BeginFrame(0);
	for(unsigned i = 0; i < fps * 2; i++){
		unsigned steps = clock.Advance(scale / fps, paused);
		for(unsigned tick = 0; tick < steps; tick++) stats.RecordTick();
		stats.BeginFrame(1.0 / fps);
	}
	return stats;
}
static void UnitTests(void) {
	for(unsigned fps : {30u, 60u, 144u, 240u}){
		CFrameTelemetry stats = Simulate(fps);
		Check(Near(stats.FPS(), fps), "render FPS is not frames / unscaled seconds");
		Check(Near(stats.PhysicsHz(), 30), "physics Hz is not actual ticks / wall seconds");
		Check(Near(stats.AverageFrameMilliseconds(), 1000.0 / fps), "frame time is not 1000 / FPS");
		Check(stats.sampleFrames > stats.sampleTicks || fps == 30, "high FPS did not decouple render and physics counts");
	}
	CFrameTelemetry half = Simulate(240, 0.5);
	Check(Near(half.FPS(), 240), "slow motion changed measured rendering FPS");
	Check(Near(half.PhysicsHz(), 16) || Near(half.PhysicsHz(), 14), "slow motion tick sample did not reflect half-speed physics");
	CFrameTelemetry paused = Simulate(240, 1, true);
	Check(Near(paused.FPS(), 240) && paused.PhysicsHz() == 0, "pause hid render FPS or fabricated physics ticks");
	CFrameTelemetry jitter;
	jitter.BeginFrame(0);
	jitter.RecordTick(); jitter.BeginFrame(0.1);
	jitter.RecordTick(); jitter.RecordTick(); jitter.BeginFrame(0.4);
	Check(Near(jitter.FPS(), 4) && Near(jitter.PhysicsHz(), 6), "jitter used an average of reciprocal frame intervals");
	Check(Near(jitter.AverageFrameMilliseconds(), 250), "jitter frame average is incorrect");
	jitter.BeginFrame(std::numeric_limits<double>::quiet_NaN());
	Check(jitter.frameSeconds == 0 && std::isfinite(jitter.FPS()), "invalid frame time poisoned telemetry");
	jitter.Reset();
	Check(jitter.FPS() == 0 && jitter.currentTicks == 0 && jitter.sampleFrames == 0, "telemetry reset retained stale rates");
	CFixedStep clock;
	clock.Advance(0.01);
	clock.Advance(0.05);
	CFixedStep::Frame frame = clock.LastFrame();
	Check(Near(frame.before, 0.01) && Near(frame.input, 0.05) && frame.steps == 1, "accumulator input telemetry does not match the scheduler");
	Check(Near(frame.before + frame.input - frame.steps * clock.StepSeconds() - frame.DroppedSeconds(), frame.remainder), "accumulator equation does not balance");
	Check(Near(frame.remainder / clock.StepSeconds(), clock.Alpha()), "alpha equation is incorrect");
	clock.Reset(); clock.Advance(10);
	frame = clock.LastFrame();
	Check(frame.input == 0.25 && frame.available == 7 && frame.steps == 5, "stall telemetry hides input clipping or capped steps");
	Check(Near(frame.DroppedSeconds(), 2.0 / 30), "stall dropped-time calculation is incorrect");
	Check(Near(frame.input - frame.steps * clock.StepSeconds() - frame.DroppedSeconds(), frame.remainder), "stall accumulator equation does not balance");
	clock.Advance(0.1, true);
	Check(clock.LastFrame().steps == 0 && clock.LastFrame().remainder == 0, "paused accumulator shows stale data");
	// Tick rates count calls to RecordTick, rather than the scheduler's requested batch.
	jitter.BeginFrame(0); jitter.RecordTick(); jitter.BeginFrame(0.5);
	Check(jitter.PhysicsHz() == 2, "telemetry fabricated unexecuted simulation steps");
}
int main(int argc, char **argv) {
	try {
		if(argc == 2){
			unsigned fps = unsigned(std::atoi(argv[1]));
			if(!fps || fps > 10000) return 2;
			CFrameTelemetry stats = Simulate(fps);
			std::cout << "{\"fps\":" << stats.FPS() << ",\"physics_hz\":" << stats.PhysicsHz()
			          << ",\"frame_ms\":" << stats.AverageFrameMilliseconds() << "}\n";
		}else{
			UnitTests();
			std::cout << checks << " telemetry checks passed\n";
		}
		return 0;
	}catch(const std::exception &error){ std::cerr << error.what() << '\n'; return 1; }
}
