#include "FixedStep.h"
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

static unsigned checks;
static void Check(bool result, const char *message)
{
	checks++;
	if(!result) throw std::runtime_error(message);
}
static bool Near(double a, double b, double tolerance = 1.0e-5)
{
	return std::fabs(a - b) <= tolerance;
}

struct Simulation
{
	CFixedStep clock;
	double previous, position, velocity;
	unsigned ticks, largestBatch;
	Simulation(void) : previous(0.0), position(0.0), velocity(1.0), ticks(0), largestBatch(0) {}
	void Frame(double seconds, bool paused = false) {
		unsigned steps = clock.Advance(seconds, paused);
		if(steps > largestBatch) largestBatch = steps;
		for(unsigned i = 0; i < steps; i++){
			previous = position;
			// Same frame-based damping/force order as the original physics paths.
			velocity = (velocity + 0.01) * 0.99;
			position += velocity * (CFixedStep::StepSeconds() * 50.0);
			ticks++;
		}
	}
	double Render(void) const {
		return previous + (position - previous) * clock.Alpha();
	}
};

static void UnitTests(void)
{
	const unsigned rates[] = {15, 30, 60, 120, 144, 240, 1000};
	Simulation baseline;
	for(unsigned i = 0; i < 300; i++) baseline.Frame(1.0 / 30.0);
	for(unsigned rate : rates){
		Simulation simulation;
		for(unsigned i = 0; i < rate * 10; i++) simulation.Frame(1.0 / rate);
		Check(simulation.ticks == 300, "render FPS changed the physics tick count");
		Check(Near(simulation.position, baseline.position), "render FPS changed position");
		Check(Near(simulation.velocity, baseline.velocity), "render FPS changed damping");
		Check(simulation.clock.Alpha() >= 0 && simulation.clock.Alpha() < 1, "alpha outside [0,1)");
	}
	CFixedStep clock;
	Check(clock.Advance(CFixedStep::StepSeconds() / 2) == 0, "partial tick ran physics");
	Check(Near(clock.Alpha(), 0.5), "partial tick was discarded");
	Check(clock.Advance(CFixedStep::StepSeconds() / 2) == 1, "exact tick boundary was missed");
	Check(clock.Advance(1.0 / 15.0) == 2, "low FPS did not catch up");
	clock.Reset();
	Check(clock.Advance(10.0) == CFixedStep::MaxSteps(), "stall exceeded the catch-up cap");
	Check(Near(clock.Alpha(), 0.5), "stall lost the sub-tick remainder");
	Check(clock.Advance(0.0) == 0, "stall left whole-tick debt");
	Check(clock.Advance(-1.0) == 0, "negative elapsed time ran physics");
	Check(clock.Advance(std::numeric_limits<double>::infinity()) == 0, "infinity ran physics");
	Check(clock.Advance(std::numeric_limits<double>::quiet_NaN()) == 0, "NaN ran physics");
	Check(clock.Advance(100.0, true) == 0, "paused frame ran physics");
	Check(clock.Alpha() == 0.0f, "pause retained simulation debt");
	Check(clock.Advance(1.0 / 240.0) == 0, "resume caught up paused wall time");
	clock.Reset();
	unsigned ticks = 0;
	for(unsigned i = 0; i < 144000; i++) ticks += clock.Advance(1.0 / 144.0);
	Check(ticks == 30000, "long-running fractional frames lost ticks");
	clock.Reset(); ticks = 0;
	for(unsigned i = 0; i < 240; i++) ticks += clock.Advance(0.5 / 240.0);
	Check(ticks == 15, "slow motion did not scale simulation time");
	CFixedStepMilliseconds milliseconds;
	unsigned totalMilliseconds = 0;
	for(unsigned i = 0; i < 30000; i++) totalMilliseconds += milliseconds.Advance();
	Check(totalMilliseconds == 1000000, "integer game clock lost fractional milliseconds");
	milliseconds.Reset();
	Check(milliseconds.Advance() == 33, "game clock did not reset cleanly");
	Simulation jitter;
	for(unsigned i = 0; i < 300; i++){
		jitter.Frame(0.005); jitter.Frame(0.010); jitter.Frame(1.0 / 30.0 - 0.015);
	}
	Check(jitter.ticks == 300 && Near(jitter.position, baseline.position), "jitter changed physics");

	CFixedTransform previous = {{0, 0, 0}, {0, 0, 0, 1}};
	CFixedTransform current = {{10, 20, 30}, {0, 0, 1, 0}};
	CFixedTransform blend = CFixedTransform::Blend(previous, current, 0.5f);
	Check(Near(blend.position[0], 5) && Near(blend.position[2], 15), "render position did not interpolate");
	Check(Near(blend.rotation[2], std::sqrt(0.5)) && Near(blend.rotation[3], std::sqrt(0.5)), "rotation did not stay normalized");
	Check(CFixedTransform::Blend(previous, current, 0).position[0] == 0, "alpha zero changed previous position");
	Check(CFixedTransform::Blend(previous, current, 1).position[0] == 10, "alpha one changed current position");
	Check(CFixedTransform::Blend(previous, current, -1).position[0] == 0, "negative alpha extrapolated");
	Check(CFixedTransform::Blend(previous, current, 2).position[0] == 10, "large alpha extrapolated");
	current.rotation[2] = 0; current.rotation[3] = -1;
	blend = CFixedTransform::Blend(previous, current, 0.5f);
	Check(Near(std::fabs(blend.rotation[3]), 1), "equivalent opposite-sign quaternions collapsed");
	current.position[0] = 1000;
	Check(CFixedTransform::Blend(previous, current, 0.5f).position[0] == 1000, "teleport interpolated through the map");
	Check(previous.position[0] == 0 && current.position[0] == 1000, "render blend mutated physics state");
	Simulation smooth;
	smooth.Frame(1.0 / 30.0);
	double authority = smooth.position;
	double lastRender = smooth.Render();
	for(unsigned i = 0; i < 7; i++){
		smooth.Frame(1.0 / 240.0);
		Check(smooth.Render() > lastRender, "high-FPS frames repeated the same visual position");
		Check(smooth.position == authority, "interpolation advanced authoritative physics");
		lastRender = smooth.Render();
	}
}

int main(int argc, char **argv)
{
	try {
		if(argc == 4 && std::string(argv[1]) == "--blend"){
			float alpha = float(std::atof(argv[2]));
			CFixedTransform previous = {{0, 0, 0}, {0, 0, 0, 1}};
			CFixedTransform current = {{float(std::atof(argv[3])), 0, 0}, {0, 0, 1, 0}};
			CFixedTransform rendered = CFixedTransform::Blend(previous, current, alpha);
			float norm = 0.0f;
			for(float component : rendered.rotation) norm += component * component;
			std::cout << std::setprecision(17) << "{\"render_position\":" << rendered.position[0]
			          << ",\"physics_position\":" << current.position[0] << ",\"rotation_norm\":" << norm << "}\n";
		}else if(argc >= 3 && std::string(argv[1]) == "--probe"){
			unsigned rate = unsigned(std::strtoul(argv[2], NULL, 10));
			if(!rate) throw std::runtime_error("FPS must be positive");
			std::string mode = argc >= 4 ? argv[3] : "normal";
			Simulation simulation;
			for(unsigned i = 0; i < rate * 10; i++)
				simulation.Frame((mode == "slow" ? 0.5 : 1.0) / rate);
			if(mode == "pause") simulation.Frame(20.0, true);
			if(mode == "stall") simulation.Frame(10.0);
			std::cout << std::setprecision(17) << "{\"ticks\":" << simulation.ticks
			          << ",\"position\":" << simulation.position << ",\"velocity\":" << simulation.velocity
			          << ",\"alpha\":" << simulation.clock.Alpha() << ",\"max_steps\":" << simulation.largestBatch << "}\n";
		}else{
			UnitTests();
			std::cout << checks << " unit checks passed\n";
		}
		return 0;
	}catch(const std::exception &error){
		std::cerr << error.what() << '\n';
		return 1;
	}
}
