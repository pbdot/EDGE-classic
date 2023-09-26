
#pragma once

#ifdef EDGE_ENABLE_PROFILING
	
	#include <tracy/Tracy.hpp>

	#define EDGE_ZoneNamed(varname, active) ZoneNamed(varname, active)
	#define EDGE_ZoneNamedN(varname, name, active) ZoneNamedN(varname, name, active)
	#define EDGE_ZoneNamedC(varname, color, active) ZoneNamedC(varname, color, active)
	#define EDGE_ZoneNamedNC(varname, name, color, active) ZoneNamedNC(varname, name, color, active)

	#define EDGE_ZoneScoped ZoneScoped
	#define EDGE_ZoneScopedN(name) ZoneScopedN(name)
	#define EDGE_ZoneScopedC(color) ZoneScopedC(color)
	#define EDGE_ZoneScopedNC(name, color) ZoneScopedNC(name, color)

	#define EDGE_ZoneText(txt, size) ZoneText(txt, size)
	#define EDGE_ZoneName(txt, size) ZoneName(txt, size)

	#define EDGE_TracyPlot(name, val) TracyPlot(name, val)

	#define EDGE_FrameMark FrameMark
	#define EDGE_FrameMarkNamed(name) FrameMarkNamed(name)
	#define EDGE_FrameMarkStart(name) FrameMarkStart(name)
	#define EDGE_FrameMarkEnd(name) FrameMarkEnd(name)

#else

	#define EDGE_ZoneNamed(varname, active)
	#define EDGE_ZoneNamedN(varname, name, active)
	#define EDGE_ZoneNamedC(varname, color, active)
	#define EDGE_ZoneNamedNC(varname, name, color, active)

	#define EDGE_ZoneScoped
	#define EDGE_ZoneScopedN(name)
	#define EDGE_ZoneScopedC(color)
	#define EDGE_ZoneScopedNC(name, color)

	#define EDGE_ZoneText(txt, size)
	#define EDGE_ZoneName(txt, size)

	#define EDGE_TracyPlot(name, val)

	#define EDGE_FrameMark
	#define EDGE_FrameMarkNamed(name)
	#define EDGE_FrameMarkStart(name)
	#define EDGE_FrameMarkEnd(name)

#endif

