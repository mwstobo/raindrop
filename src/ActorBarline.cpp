#include "pch.h"

#include "GameGlobal.h"
#include "ScreenGameplay.h"
#include "ActorBarline.h"
#include "GameWindow.h"

#define BLUE_CONSTANT (200.0 / 255.0)

namespace Game {
	namespace dotcur {

		ActorBarline::ActorBarline(ScreenGameplay *_Parent) : Sprite()
		{
			Parent = _Parent;
			Centered = true;
			ColorInvert = false;
			AnimationTime = 0;
			AnimationProgress = 0;
			AffectedByLightning = true;
		}

		void ActorBarline::Init(float Offset)
		{
			AnimationProgress = AnimationTime = Offset;

			SetPosition(Parent->GetScreenOffset(0.5).x, ScreenOffset + 3 * PlayfieldWidth / 4);
			SetWidth(PlayfieldWidth);

			Red = Green = 0;
			Blue = 200.f / 255.f;
			Alpha = 0;
		}

#define RadioThreshold 0.9

		void ActorBarline::Run(double TimeDelta, double Ratio)
		{
			if (AnimationProgress > 0)
			{
				float PosY = pow(AnimationProgress / AnimationTime, 2) * PlayfieldHeight + ScreenOffset;

				Alpha = 1 - AnimationProgress;

				SetPositionY(PosY);
				AnimationProgress -= TimeDelta;

				if (AnimationProgress <= 0)
					AnimationProgress = 0;

				return;
			}

			if (Parent->GetMeasure() % 2)
			{
				Red = 1;
				Blue = Green = 0;

				if (Ratio > RadioThreshold) // Red to blue
				{
					double diff = Ratio - RadioThreshold;
					double duration = (1 - RadioThreshold);

					Red = LerpRatio(1.0, 0.0, diff, duration);
					Blue = LerpRatio(0.0, BLUE_CONSTANT, diff, duration);
				}
			}
			else
			{
				Red = 0.0;
				Blue = 200.f / 255.f;

				if (Ratio > RadioThreshold) // Blue to red
				{
					float diff = Ratio - RadioThreshold;
					double duration = (1 - RadioThreshold);

					Red = LerpRatio(0.0, 1.0, diff, duration);
					Blue = LerpRatio(BLUE_CONSTANT, 0.0, diff, duration);
				}
			}

			SetPositionY(Ratio * (float)PlayfieldHeight);

			if (Parent->GetMeasure() % 2)
				SetPositionY(PlayfieldHeight - GetPosition().y);

			SetPositionY(GetPosition().y + ScreenOffset);

			// assert(GetPosition().y > ScreenOffset);
		}
	}
}