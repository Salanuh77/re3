import json
import math
import subprocess

from behave import given, when, then


def run(context, fps, mode="normal"):
    executable = context.config.userdata["physics_probe"]
    return json.loads(subprocess.check_output([executable, "--probe", str(fps), mode], text=True))


@given("a reference simulation rendered at 30 FPS")
def reference(context):
    context.reference = run(context, 30)


@when("the simulation runs for 10 seconds at {fps:d} FPS")
def simulate(context, fps):
    context.result = run(context, fps)


@when("a 240 FPS simulation is paused for 20 seconds")
def pause(context):
    context.result = run(context, 240, "pause")


@when("a 240 FPS simulation stalls for 10 seconds")
def stall(context):
    context.result = run(context, 240, "stall")


@when("a 240 FPS simulation runs at half speed")
def slow(context):
    context.result = run(context, 240, "slow")


@then("exactly {ticks:d} physics ticks have run")
def count(context, ticks):
    assert context.result["ticks"] == ticks, context.result


@then("its position and velocity match the reference")
def equal_motion(context):
    for key in ("position", "velocity"):
        assert math.isclose(context.result[key], context.reference[key], abs_tol=1e-8), context.result


@then("the interpolation fraction is between zero and one")
def alpha(context):
    assert 0 <= context.result["alpha"] < 1, context.result


@then("the interpolation fraction is zero")
def zero_alpha(context):
    assert context.result["alpha"] == 0, context.result


@then("no render frame runs more than {limit:d} physics ticks")
def cap(context, limit):
    assert context.result["max_steps"] <= limit, context.result


@when("a transform at position {position:d} is rendered with interpolation fraction {alpha:f}")
def blend(context, position, alpha):
    executable = context.config.userdata["physics_probe"]
    context.result = json.loads(subprocess.check_output(
        [executable, "--blend", str(alpha), str(position)], text=True))


@then("its displayed position is {position:d}")
def display_position(context, position):
    assert math.isclose(context.result["render_position"], position, abs_tol=1e-6), context.result


@then("its authoritative physics position remains {position:d}")
def physics_position(context, position):
    assert context.result["physics_position"] == position, context.result


@then("its displayed rotation is normalized")
def rotation_normalized(context):
    assert math.isclose(context.result["rotation_norm"], 1, abs_tol=1e-6), context.result


@when("the camera looks at yaw {yaw:d} and pitch {pitch:d} degrees")
def camera_look(context, yaw, pitch):
    context.camera_result = subprocess.run(
        [context.config.userdata["camera_probe"], str(yaw), str(pitch)],
        capture_output=True, text=True)


@then("the camera conversion preserves its viewing direction and horizon")
def camera_preserved(context):
    assert context.camera_result.returncode == 0, context.camera_result.stderr


@when("live telemetry observes gameplay rendered at {fps:d} FPS")
def telemetry_observes(context, fps):
    result = subprocess.run(
        [context.config.userdata["telemetry_probe"], str(fps)],
        capture_output=True, text=True, check=True)
    context.telemetry = json.loads(result.stdout)


@then("the telemetry reports {fps:d} render FPS and 30 physics Hz")
def telemetry_rates(context, fps):
    assert math.isclose(context.telemetry["fps"], fps, rel_tol=1e-5)
    assert math.isclose(context.telemetry["physics_hz"], 30, rel_tol=1e-5)


@then("the average frame time equals 1000 divided by {fps:d} milliseconds")
def telemetry_frame_time(context, fps):
    assert math.isclose(context.telemetry["frame_ms"], 1000 / fps, rel_tol=1e-5)
