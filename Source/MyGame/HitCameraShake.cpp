// Fill out your copyright notice in the Description page of Project Settings.


#include "HitCameraShake.h"

UHitCameraShake::UHitCameraShake()
{
	OscillationDuration = 0.3f; // Duration in seconds of screen shake
	OscillationBlendInTime = 1.f; // Duration of the blend-in, where the oscillation scales from 0 to 1
	OscillationBlendOutTime = 0.2f; // Duration of the blend-out, where the oscillation scales from 1 to 0.

	//direction in which we shake the camera (forward-Pitch and sideways-Yaw)
	RotOscillation.Pitch.Amplitude = 2.f;
	RotOscillation.Pitch.Frequency = 100.f;
	RotOscillation.Yaw.Amplitude = 2.f;
	RotOscillation.Yaw.Frequency = 100.f;
	RotOscillation.Roll.Amplitude = 2.f;
	RotOscillation.Roll.Frequency = 100.f;

	FOVOscillation.Amplitude = 10.f;
	FOVOscillation.Frequency = 1000.f;
}