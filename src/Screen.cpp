#include "Global.h"
#include "SceneEnvironment.h"
#include "Screen.h"

Screen::Screen(GString Name) 
{  
	Parent = NULL;
	Running = false;
	Next = 0;
	ScreenTime = 0;
	IntroDuration = 0;
	ScreenState = StateRunning;
	Animations = make_shared<SceneEnvironment>(Name.c_str());
}

Screen::Screen(GString Name, Screen *_Parent)
{
	Parent = _Parent;
	Running = false;
	Next = 0;
	ScreenTime = 0;
	IntroDuration = 0;
	ScreenState = StateRunning;
	Animations = make_shared<SceneEnvironment>(Name.c_str());
}

Screen::~Screen() {}

bool Screen::HandleTextInput(int codepoint)
{
	if (Next)
		return Next->HandleTextInput(codepoint);
	else return false;
}

void Screen::Close()
{
	Cleanup();
	Running = false;
	if (Next)
		Next->Close();
}

void Screen::LoadThreadInitialization()
{
	// virtual
}

void Screen::MainThreadInitialization()
{
	// virtual
}

void Screen::ChangeState(Screen::EScreenState NewState)
{
	ScreenState = NewState;

	switch (NewState) {
	case StateIntro:
		OnIntroBegin();
		break;
	case StateExit:
		OnExitBegin();
		break;
	default:
		break;
	}

	TransitionTime = 0;
}

bool Screen::IsScreenRunning()
{
	return Running;
}

bool Screen::RunNested(float delta)
{
	if (!Next)
		return false;

	if (Next->Update(delta))
		return true;
	else // The screen's done?
	{
		// It's not null- so we'll delete it.
		Next->Cleanup();
		delete Next;
		Next = 0;
		return false;
	}

	// Reaching this point SHOULDn't happen.
	return false;
}

Screen* Screen::GetTop()
{
	if (Next) return Next->GetTop();
	else return this;
}

double Screen::GetScreenTime()
{
	return ScreenTime;
}

bool Screen::Update(float delta)
{
	ScreenTime += delta;

	if (ScreenState == StateIntro)
	{
		float Frac;
		TransitionTime += delta;

		if (TransitionTime < IntroDuration)
			Frac = Clamp(TransitionTime / IntroDuration, 0.0, 1.0);
		else
		{
			Frac = 1;
			ScreenState = StateRunning;
		}

		return RunIntro(Frac, delta);
	}else if (ScreenState == StateExit)
	{
		float Frac;

		TransitionTime += delta;
		if (TransitionTime < ExitDuration)
			Frac = Clamp(TransitionTime / ExitDuration, 0.0, 1.0);
		else
		{
			Frac = 1;
			ScreenState = StateRunning;
		}

		/* 
			StateExit can still go back to the "StateRunning" state
			This way it can be used for transitions.
		*/
		return RunExit(Frac, delta); 
	}else
		return Run(delta);
}

void Screen::Init()
{
	LoadThreadInitialization();
	MainThreadInitialization();
}

bool Screen::RunIntro(float Fraction, float Delta)
{
	Animations->RunIntro(Fraction, Delta);

	if (Fraction == 1)
		OnIntroEnd();

	return Running;
}

bool Screen::RunExit(float Fraction, float Delta)
{
	Animations->RunExit(Fraction, Delta);

	if (Fraction == 1)
		OnExitEnd();

	return Running;
}
bool Screen::HandleInput(int32 key, KeyEventType state, bool isMouseInput) 
{ 
	if (Next && Next->IsScreenRunning())
		return Next->HandleInput(key, state, isMouseInput);

	return false;
}

bool Screen::HandleScrollInput(double xOff, double yOff) 
{
	if (Next && Next->IsScreenRunning())
		return Next->HandleScrollInput(xOff, yOff);

	return false;
}
void Screen::OnIntroBegin()
{
	Animations->DoEvent("OnIntroBegin");
}

void Screen::OnIntroEnd()
{
	Animations->DoEvent("OnIntroEnd");
}

void Screen::OnExitBegin()
{
	Animations->DoEvent("OnExitBegin");
}

void Screen::OnExitEnd()
{
	Animations->DoEvent("OnExitEnd");
}

void Screen::OnRunningBegin()
{
	Animations->DoEvent("OnRunningBegin");
}

void Screen::Cleanup() { /* stub */ }

void Screen::Invalidate() { /* stub */ }