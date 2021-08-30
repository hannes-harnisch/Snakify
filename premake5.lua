output_dir				= '%{cfg.buildcfg}_%{cfg.architecture}_%{cfg.system}'

workspace 'Snakify'
	architecture		'x64'
	configurations		{ 'Debug', 'Release' }
	flags				{ 'MultiProcessorCompile' }
	startproject		'Snakify'
	language			'C++'
	cppdialect			'C++Latest'
	warnings			'Extra'
	conformancemode		'On'
	staticruntime		'On'
	files				{ 'Snakify/Main.cc' }
	objdir				( '.bin_int/' .. output_dir .. '/%{prj.name}' )
	targetdir			( '.bin/'	  .. output_dir .. '/%{prj.name}' )
	debugdir			( '.bin/'	  .. output_dir .. '/%{prj.name}' )

	filter 'configurations:Debug'
		runtime			'Debug'
		symbols			'On'
		defines			'DEBUG'

	filter 'configurations:Release'
		runtime			'Release'
		optimize		'Speed'
		flags			{ 'LinkTimeOptimization' }

project 'Snakify'
	location			'Snakify'
	entrypoint			'mainCRTStartup'
	kind				'ConsoleApp'