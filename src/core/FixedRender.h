#pragma once

#ifdef FIXED_TIME_STEP
class CFixedRender
{
public:
	static void Reset(void);
	static void BeforeStep(void);
	static void AfterStep(void);
	static void RestoreLights(void);
	static void Begin(float alpha);
	static void End(void);
};

// Restores authoritative RW transforms on every Idle exit, including goto popret.
class CFixedRenderScope
{
public:
	~CFixedRenderScope(void) { CFixedRender::End(); }
};
#endif
