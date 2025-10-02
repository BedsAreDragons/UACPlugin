# Upper Area Control PlugIn [![Build status](https://ci.appveyor.com/api/projects/status/github/BedsAreDragons/UACPlugin?svg=true)](https://ci.appveyor.com/project/BedsAreDragons/UACPlugin)

<p align="center">
	<img alt="UAC PlugIn header" src="https://github.com/BedsAreDragons/UACPlugin/blob/master/docs/img/image.png?raw=true"/>
</p>

This is my continued version that I am going to continue updating as pierr3 has stopped updating his version. I plan to add:
* TBS FTDi's
* Tags based on NULATSA Systems
* Anything else I decide to come up with

This plugin simulates an ATC HMI resembling systems used by large European Air Traffic Control providers, specially en route. The plugin supports an en route mode and an approach mode, allowing for full topdown control (excluding ground control). It features a number of advanced systems, while still being light and usable:
 * En route control & approach control mode
 * Custom-drawn tags, with custom, realistic behaviour
 * Realistic filtering (by levels, VFRs, Primary)
 * Precise separation tool (min distance), custom STCA (no alert sound), custom MTCD
 * Anti-overlap algorithm based on a weighted grid system, meaning tags automatically move to not overlap
 * Full support for coordination mechanisms from EuroScope
 * Display of max and minimum speeds
 * Display of mode S reported heading
 
 Download the most recent development (nightly) build here:
 
 <https://ci.appveyor.com/api/projects/pierr3/UACPlugin/artifacts/UAC-nightly.zip>
