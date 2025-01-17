#include "PIDReportHandler.h"
#if 1
#define DEBUG_PRINT(x)	Serial.print(x)
#define DEBUG_PRINTLN(x)	Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

PIDReportHandler::PIDReportHandler() 
{
	nextEID = 1;
	devicePaused = 0;
	memset(&g_EffectStates, 0, sizeof(g_EffectStates));
}

PIDReportHandler::~PIDReportHandler() 
{
	FreeAllEffects();
}

void PIDReportHandler::EnableDefaultEffect(const TEffectState &effect)
{
	memcpy(&g_EffectStates[0], &effect, sizeof(TEffectState));
	const uint8_t id = GetNextFreeEffect();
	memcpy(&g_EffectStates[id], &g_EffectStates[0], sizeof(TEffectState));
	if (id != 1)
	{
		DEBUG_PRINT("nextEID != 1: ");
		DEBUG_PRINTLN(id);
	}
	StartEffect(id);
	DEBUG_PRINT("Enabled default effect: ");
	DEBUG_PRINTLN(g_EffectStates[id].duration);
}

void PIDReportHandler::PrintEffect(uint8_t id)
{
	DEBUG_PRINT("Effect ");
	DEBUG_PRINT(id);
	DEBUG_PRINTLN(":");
	DEBUG_PRINT("  state ");
	DEBUG_PRINTLN(g_EffectStates[id].state);
	DEBUG_PRINT("  effectType ");
	DEBUG_PRINTLN(g_EffectStates[id].effectType);
	DEBUG_PRINT("  offset ");
	DEBUG_PRINTLN(g_EffectStates[id].offset);
	DEBUG_PRINT("  gain ");
	DEBUG_PRINTLN(g_EffectStates[id].gain);
	DEBUG_PRINT("  attackLevel ");
	DEBUG_PRINTLN(g_EffectStates[id].attackLevel);
	DEBUG_PRINT("  fadeLevel ");
	DEBUG_PRINTLN(g_EffectStates[id].fadeLevel);
	DEBUG_PRINT("  magnitude ");
	DEBUG_PRINTLN(g_EffectStates[id].magnitude);
	DEBUG_PRINT("  enableAxis ");
	DEBUG_PRINTLN(g_EffectStates[id].enableAxis);
	DEBUG_PRINT("   directionX ");
	DEBUG_PRINTLN(g_EffectStates[id].directionX);
	DEBUG_PRINT("  directionY ");
	DEBUG_PRINTLN(g_EffectStates[id].directionY);
	DEBUG_PRINT("  phase ");
	DEBUG_PRINTLN(g_EffectStates[id].phase);
	DEBUG_PRINT("  startMagnitude ");
	DEBUG_PRINTLN(g_EffectStates[id].startMagnitude);
	DEBUG_PRINT("  endMagnitude ");
	DEBUG_PRINTLN(g_EffectStates[id].endMagnitude);
	DEBUG_PRINT("  period ");
	DEBUG_PRINTLN(g_EffectStates[id].period);
	DEBUG_PRINT("  duration ");
	DEBUG_PRINTLN(g_EffectStates[id].duration);
	DEBUG_PRINT("  fadeTime ");
	DEBUG_PRINTLN(g_EffectStates[id].fadeTime);
	DEBUG_PRINT("  attackTime ");
	DEBUG_PRINTLN(g_EffectStates[id].attackTime);
	DEBUG_PRINT("  elapsedTime ");
	DEBUG_PRINTLN(g_EffectStates[id].elapsedTime);
//	DEBUG_PRINT("startTime ");
//	DEBUG_PRINTLN(g_EffectStates[id].startTime);
	DEBUG_PRINT("  cpOffset ");
	DEBUG_PRINTLN(g_EffectStates[id].conditions[0].cpOffset);
	DEBUG_PRINT("  positiveCoefficient ");
	DEBUG_PRINTLN(g_EffectStates[id].conditions[0].positiveCoefficient);
	DEBUG_PRINT("  negativeCoefficient ");
	DEBUG_PRINTLN(g_EffectStates[id].conditions[0].negativeCoefficient);
	DEBUG_PRINT("  positiveSaturation ");
	DEBUG_PRINTLN(g_EffectStates[id].conditions[0].positiveSaturation);
	DEBUG_PRINT("  negativeSaturation ");
	DEBUG_PRINTLN(g_EffectStates[id].conditions[0].negativeSaturation);
	DEBUG_PRINT("  deadBand ");
	DEBUG_PRINTLN(g_EffectStates[id].conditions[0].deadBand);
}

uint8_t PIDReportHandler::GetNextFreeEffect(void)
{
	if (nextEID == MAX_EFFECTS)
		return 0;

	uint8_t id = nextEID++;

	while (g_EffectStates[nextEID].state != 0)
	{
		if (nextEID >= MAX_EFFECTS)
			break;  // the last spot was taken
		nextEID++;
	}

	g_EffectStates[id].state = MEFFECTSTATE_ALLOCATED;

	return id;
}

void PIDReportHandler::StopAllEffects(void)
{
	for (uint8_t id = 0; id <= MAX_EFFECTS; id++)
		StopEffect(id);
}

void PIDReportHandler::StartEffect(uint8_t id)
{
	if (id > MAX_EFFECTS)
		return;
	g_EffectStates[id].state = MEFFECTSTATE_PLAYING;
	g_EffectStates[id].elapsedTime = 0;
	// TODO: Casting 32 to 64 bit is probably not right?
	g_EffectStates[id].startTime = (uint64_t)millis();
}

void PIDReportHandler::StopEffect(uint8_t id)
{
	if (id > MAX_EFFECTS)
		return;
	g_EffectStates[id].state = MEFFECTSTATE_ALLOCATED;
}

void PIDReportHandler::FreeEffect(uint8_t id)
{
	if (id > MAX_EFFECTS)
		return;
	g_EffectStates[id].state = 0;
	if (id < nextEID)
		nextEID = id;
	pidBlockLoad.ramPoolAvailable += SIZE_EFFECT;
}

void PIDReportHandler::FreeAllEffects(void)
{
	nextEID = 1;
	for (uint8_t i = 1; i < MAX_EFFECTS + 1; ++i)
	{
		memset(&g_EffectStates[i], 0, sizeof(TEffectState));
	}
	if (g_EffectStates[0].state != MEFFECTSTATE_FREE)
	{
		// Default effect is enabled.
		memcpy(&g_EffectStates[1], &g_EffectStates[0], sizeof(TEffectState));
		nextEID += 1;
		StartEffect(1);
	}
	pidBlockLoad.ramPoolAvailable = MEMORY_SIZE;
	DEBUG_PRINTLN("Freed All Effects");
}

void PIDReportHandler::EffectOperation(USB_FFBReport_EffectOperation_Output_Data_t* data)
{
	if (data->operation == 1)
	{ // Start
		if (data->loopCount > 0) g_EffectStates[data->effectBlockIndex].duration *= data->loopCount;
		if (data->loopCount == 0xFF) g_EffectStates[data->effectBlockIndex].duration = USB_DURATION_INFINITE;
		StartEffect(data->effectBlockIndex);
	}
	else if (data->operation == 2)
	{ // StartSolo

	  // Stop all first
		StopAllEffects();
		// Then start the given effect
		StartEffect(data->effectBlockIndex);
	}
	else if (data->operation == 3)
	{ // Stop
		StopEffect(data->effectBlockIndex);
	}
	else
	{
	}
}

void PIDReportHandler::BlockFree(USB_FFBReport_BlockFree_Output_Data_t* data)
{
	uint8_t eid = data->effectBlockIndex;

	if (eid == 0xFF)
	{ // all effects
		FreeAllEffects();
	}
	else
	{
		FreeEffect(eid);
	}
}

void PIDReportHandler::DeviceControl(USB_FFBReport_DeviceControl_Output_Data_t* data)
{

	uint8_t control = data->control;

	if (control == 0x01)
	{ // 1=Enable Actuators
		pidState.status |= 2;
	}
	else if (control == 0x02)
	{ // 2=Disable Actuators
		pidState.status &= ~(0x02);
	}
	else if (control == 0x03)
	{ // 3=Stop All Effects
		StopAllEffects();
	}
	else if (control == 0x04)
	{ //  4=Reset
		FreeAllEffects();
	}
	else if (control == 0x05)
	{ // 5=Pause
		devicePaused = 1;
	}
	else if (control == 0x06)
	{ // 6=Continue
		devicePaused = 0;
	}
	else if (control & (0xFF - 0x3F))
	{
	}
}

void PIDReportHandler::DeviceGain(USB_FFBReport_DeviceGain_Output_Data_t* data)
{
	deviceGain.gain = data->gain;
}

void PIDReportHandler::SetCustomForce(USB_FFBReport_SetCustomForce_Output_Data_t* data)
{
}

void PIDReportHandler::SetCustomForceData(USB_FFBReport_SetCustomForceData_Output_Data_t* data)
{
}

void PIDReportHandler::SetDownloadForceSample(USB_FFBReport_SetDownloadForceSample_Output_Data_t* data)
{
}

void PIDReportHandler::SetEffect(USB_FFBReport_SetEffect_Output_Data_t* data)
{
	volatile TEffectState* effect = &g_EffectStates[data->effectBlockIndex];

	effect->duration = data->duration;
	effect->directionX = data->directionX;
	effect->directionY = data->directionY;
	effect->effectType = data->effectType;
	effect->gain = data->gain;
	effect->enableAxis = data->enableAxis;
	DEBUG_PRINT("dX: ");
	DEBUG_PRINT(effect->directionX);
	DEBUG_PRINT(" dX: ");
	DEBUG_PRINT(effect->directionY);
	DEBUG_PRINT(" eT: ");
	DEBUG_PRINT(effect->effectType);
	DEBUG_PRINT(" eA: ");
	DEBUG_PRINTLN(effect->enableAxis);
}

void PIDReportHandler::SetEnvelope(USB_FFBReport_SetEnvelope_Output_Data_t* data, volatile TEffectState* effect)
{
	effect->attackLevel = data->attackLevel;
	effect->fadeLevel = data->fadeLevel;
	effect->attackTime = data->attackTime;
	effect->fadeTime = data->fadeTime;
}

void PIDReportHandler::SetCondition(USB_FFBReport_SetCondition_Output_Data_t* data, volatile TEffectState* effect)
{
	uint8_t axis = data->parameterBlockOffset; 
    effect->conditions[axis].cpOffset = data->cpOffset;
    effect->conditions[axis].positiveCoefficient = data->positiveCoefficient;
    effect->conditions[axis].negativeCoefficient = data->negativeCoefficient;
    effect->conditions[axis].positiveSaturation = data->positiveSaturation;
    effect->conditions[axis].negativeSaturation = data->negativeSaturation;
    effect->conditions[axis].deadBand = data->deadBand;
	effect->conditionBlocksCount++;
}

void PIDReportHandler::SetPeriodic(USB_FFBReport_SetPeriodic_Output_Data_t* data, volatile TEffectState* effect)
{
	effect->magnitude = data->magnitude;
	effect->offset = data->offset;
	effect->phase = data->phase;
	effect->period = data->period;

	DEBUG_PRINT(" m: ");
	DEBUG_PRINTLN(effect->magnitude);
	DEBUG_PRINT(" o: ");
	DEBUG_PRINTLN(effect->offset);
	DEBUG_PRINT(" ph: ");
	DEBUG_PRINTLN(effect->phase);
	DEBUG_PRINT(" p: ");
	DEBUG_PRINTLN(effect->period);
}

void PIDReportHandler::SetConstantForce(USB_FFBReport_SetConstantForce_Output_Data_t* data, volatile TEffectState* effect)
{
	//  ReportPrint(*effect);
	effect->magnitude = data->magnitude;
}

void PIDReportHandler::SetRampForce(USB_FFBReport_SetRampForce_Output_Data_t* data, volatile TEffectState* effect)
{
	effect->startMagnitude = data->startMagnitude;
	effect->endMagnitude = data->endMagnitude;
}

void PIDReportHandler::CreateNewEffect(USB_FFBReport_CreateNewEffect_Feature_Data_t* inData)
{
	pidBlockLoad.reportId = 6;
	pidBlockLoad.effectBlockIndex = GetNextFreeEffect();

	if (pidBlockLoad.effectBlockIndex == 0)
	{
		pidBlockLoad.loadStatus = 2;    // 1=Success,2=Full,3=Error
	}
	else
	{
		pidBlockLoad.loadStatus = 1;    // 1=Success,2=Full,3=Error

		volatile TEffectState* effect = &g_EffectStates[pidBlockLoad.effectBlockIndex];

		memset((void*)effect, 0, sizeof(TEffectState));
		effect->state = MEFFECTSTATE_ALLOCATED;
		pidBlockLoad.ramPoolAvailable -= SIZE_EFFECT;
	}
}

void PIDReportHandler::UppackUsbData(uint8_t* data, uint16_t len)
{
	DEBUG_PRINT("len:");
	DEBUG_PRINTLN(len);

	uint8_t effectId = data[1]; // effectBlockIndex is always the second byte.

	DEBUG_PRINT("id:");
	DEBUG_PRINTLN(effectId);

	switch (data[0])    // reportID
	{
	case 1:
		DEBUG_PRINTLN("SetEffect");
		SetEffect((USB_FFBReport_SetEffect_Output_Data_t*)data);
		break;
	case 2:
		DEBUG_PRINTLN("SetEnvelop");
		SetEnvelope((USB_FFBReport_SetEnvelope_Output_Data_t*)data, &g_EffectStates[effectId]);
		break;
	case 3:
		DEBUG_PRINTLN("SetCondition");
		SetCondition((USB_FFBReport_SetCondition_Output_Data_t*)data, &g_EffectStates[effectId]);
		break;
	case 4:
		DEBUG_PRINTLN("SetPeriodic");
		SetPeriodic((USB_FFBReport_SetPeriodic_Output_Data_t*)data, &g_EffectStates[effectId]);
		break;
	case 5:
		DEBUG_PRINTLN("SetConstantForce");
		SetConstantForce((USB_FFBReport_SetConstantForce_Output_Data_t*)data, &g_EffectStates[effectId]);
		PrintEffect(effectId);
		break;
	case 6:
		DEBUG_PRINTLN("SetRampForce");
		SetRampForce((USB_FFBReport_SetRampForce_Output_Data_t*)data, &g_EffectStates[effectId]);
		break;
	case 7:
		DEBUG_PRINTLN("SetCustomForceData");
		SetCustomForceData((USB_FFBReport_SetCustomForceData_Output_Data_t*)data);
		break;
	case 8:
		DEBUG_PRINTLN("SetDownloadForceSample");
		SetDownloadForceSample((USB_FFBReport_SetDownloadForceSample_Output_Data_t*)data);
		break;
	case 9:
		break;
	case 10:
		DEBUG_PRINTLN("EffectOperation");
		EffectOperation((USB_FFBReport_EffectOperation_Output_Data_t*)data);
		break;
	case 11:
		DEBUG_PRINTLN("BlockFree");
		BlockFree((USB_FFBReport_BlockFree_Output_Data_t*)data);
		break;
	case 12:
		DEBUG_PRINTLN("DeviceControl");
		DeviceControl((USB_FFBReport_DeviceControl_Output_Data_t*)data);
		break;
	case 13:
		DEBUG_PRINTLN("DeviceGain");
		DeviceGain((USB_FFBReport_DeviceGain_Output_Data_t*)data);
		break;
	case 14:
		DEBUG_PRINTLN("SetCustomForce");
		SetCustomForce((USB_FFBReport_SetCustomForce_Output_Data_t*)data);
		break;
	default:
		break;
	}
}

uint8_t* PIDReportHandler::getPIDPool()
{
	FreeAllEffects();

	pidPoolReport.reportId = 7;
	pidPoolReport.ramPoolSize = MEMORY_SIZE;
	pidPoolReport.maxSimultaneousEffects = MAX_EFFECTS;
	pidPoolReport.memoryManagement = 3;
	return (uint8_t*)& pidPoolReport;
}

uint8_t* PIDReportHandler::getPIDBlockLoad()
{
	return (uint8_t*)& pidBlockLoad;
}

uint8_t* PIDReportHandler::getPIDStatus()
{
	return (uint8_t*)& pidState;
}