#include "pch.h"

#include "GameGlobal.h"
#include "GameState.h"
#include "ScreenEdit.h"
#include "GameWindow.h"
#include "ImageLoader.h"
#include "Audio.h"

SoundSample *SavedSound = NULL;

double Fracs[] = {
    1,
    2,
    3,
    4,
    6,
    8,
    12,
    16,
    24,
    32,
    48,
    64,
    96,
    192
};

namespace Game {
	namespace dotcur {

		ScreenEdit::ScreenEdit()
			: ScreenGameplay()
		{
			ShouldChangeScreenAtEnd = false; // So it doesn't go into screen evaluation.
			CurrentFraction = 0;
			Measure = 0;
			EditScreenState = Editing;

			GhostObject.SetImage(GameState::GetInstance().GetSkinImage("hitcircle.png"));
			GhostObject.Alpha = 0.7f;
			GhostObject.Centered = true;
			GhostObject.SetSize(CircleSize);

			EditInfo.LoadSkinFontImage("font.tga", Vec2(6, 15), Vec2(8, 16), Vec2(6, 15), 0);
			EditMode = true;
			HeldObject = NULL;
			Mode = Select;
			GridEnabled = false;
			GridCellSize = 16;

			CurrentTotalFraction = 0;
		}

		void ScreenEdit::Cleanup()
		{
			ScreenGameplay::Cleanup();
		}

		void ScreenEdit::Init(dotcur::Song *Other)
		{
			if (Other != NULL)
			{
				if (Other->Difficulties.size() == 0) // No difficulties? Create a new one.
					Other->Difficulties.push_back(new dotcur::Difficulty());

				ScreenGameplay::Init(Other, 0);
				NotesInMeasure.clear();

				if (!Other->Difficulties[0]->Timing.size())
					Other->Difficulties[0]->Timing.push_back(TimingSegment());
			}

			if (!SavedSound)
			{
				SavedSound = new SoundSample();
				SavedSound->Open((GameState::GetInstance().GetSkinFile("save.ogg")).c_str());
			}

			OffsetPrompt.SetPrompt("Please insert offset.");
			BPMPrompt.SetPrompt("Please insert BPM.");
			OffsetPrompt.SetFont(&EditInfo);
			BPMPrompt.SetFont(&EditInfo);

			OffsetPrompt.SetOpen(false);
			BPMPrompt.SetOpen(false);
		}

		void ScreenEdit::StartPlaying(int32_t _Measure)
		{
			ScreenGameplay::Init(MySong, 0);
			ScreenGameplay::ResetNotes();
			ScreenGameplay::MainThreadInitialization();

			Measure = _Measure;
			seekTime(TimeAtBeat(CurrentDiff->Timing, CurrentDiff->Offset, Measure * MySong->MeasureLength));
			savedMeasure = Measure;
		}

		void ScreenEdit::SwitchPreviewMode()
		{
			MySong->Process(false);
			if (EditScreenState == Editing) // if we're editing, start playing the song
			{
				EditScreenState = Playing;
				StartPlaying(Measure);
			}
			else if (EditScreenState == Playing) // if we're playing, go back to editing
			{
				EditScreenState = Editing;
				Measure = savedMeasure;
				stopMusic();
				RemoveTrash();
			}
		}

		void ScreenEdit::DecreaseCurrentFraction()
		{
			if (HeldObject)
			{
				HeldObject->hold_duration -= 4.0 / Fracs[CurrentTotalFraction];
				if (HeldObject->hold_duration < 0)
					HeldObject->hold_duration = 0;
				MySong->Process(false);
			}

			if (CurrentFraction || Measure > 0)
			{
				CurrentFraction--;

				if (CurrentFraction / Fracs[CurrentTotalFraction] > 1) // overflow
				{
					CurrentFraction = Fracs[CurrentTotalFraction] - 1;

					if (Measure > 0) // Go back a measure
						Measure--;
				}
			}
		}

		void ScreenEdit::IncreaseCurrentFraction()
		{
			if (!CurrentDiff->Measures.size())
				return;

			if (HeldObject)
			{
				HeldObject->hold_duration += 4.0 / Fracs[CurrentTotalFraction];
				MySong->Process(false);
			}

			CurrentFraction++;

			if (CurrentFraction / Fracs[CurrentTotalFraction] >= 1)
			{
				CurrentFraction = 0;
				if (Measure + 1 < CurrentDiff->Measures.size()) // Advance a measure
					Measure++;
			}
		}

		void ScreenEdit::SaveChart()
		{
			std::filesystem::path DefaultPath = MySong->ChartFilename.string().length() ? MySong->ChartFilename : MySong->SongDirectory / "chart.dcf";
			MySong->Repack();
			MySong->Save(DefaultPath.string().c_str());
			SavedSound->Play();
			MySong->Process();
		}

		void ScreenEdit::InsertMeasure()
		{
			CurrentDiff->Measures.resize(CurrentDiff->Measures.size() + 1);
			Measure = CurrentDiff->Measures.size() - 1;
			CurrentFraction = 0;
		}

		void ScreenEdit::OnMousePress(KeyType tkey)
		{
			if (Mode != Select)
			{
				GameObject &G = GetObject();
				if (tkey == KT_Select)
				{
					uint32_t selFrac = CurrentFraction / Fracs[CurrentTotalFraction] * 192;

					G.SetPositionX(GhostObject.GetPosition().x);
					G.Fraction = (double)CurrentFraction / Fracs[CurrentTotalFraction];

					if (Mode == Hold)
					{
						HeldObject = &G;
						HeldObject->endTime = 0;
						HeldObject->hold_duration = 0;
					}
				}
				else
				{
					HeldObject = NULL;
					G.SetPositionX(0);
					G.endTime = 0;
					G.hold_duration = 0;
				}

				MySong->Process(false);
				ScreenGameplay::ResetNotes();
			}
		}

		void ScreenEdit::OnMouseRelease(KeyType tkey)
		{
			if (Mode == Hold && tkey == KT_Select)
				HeldObject = NULL;
		}

		bool ScreenEdit::HandleInput(int32_t key, KeyEventType code, bool isMouseInput)
		{
			KeyType tkey = BindingsManager::TranslateKey(key);

			if (!isMouseInput)
				if (key == 'P') // pressed p?
				{
					SwitchPreviewMode();
					return true;
				}

			// Playing mode input
			if (EditScreenState == Playing)
			{
				ScreenGameplay::HandleInput(key, code, isMouseInput);
				return true;
			}

			// Editor mode input
			if (!isMouseInput)
			{
				if (key == 'P') // pressed P
					SwitchPreviewMode();

				int R = OffsetPrompt.HandleInput(key, code, isMouseInput);

				if (R == 1)
					return true;
				else if (R == 2)
				{
					if (Utility::IsNumeric(OffsetPrompt.GetContents().c_str()))
					{
						CurrentDiff->Offset = atof(OffsetPrompt.GetContents().c_str());
					}
					return true;
				}

				R = BPMPrompt.HandleInput(key, code, isMouseInput);

				if (R == 1)
					return true;
				else if (R == 2)
				{
					if (Utility::IsNumeric(BPMPrompt.GetContents().c_str()))
					{
						CurrentDiff->Timing[0].Value = atof(BPMPrompt.GetContents().c_str());
						MySong->Process(false);
					}
					return true;
				}

				if (code == KE_PRESS)
				{
					switch (tkey)
					{
					case KT_Right:              IncreaseCurrentFraction(); return true;
					case KT_Left:               DecreaseCurrentFraction(); return true;
					case KT_Escape:             Running = false; return true;
					case KT_FractionDec:        if (CurrentDiff->Measures.size()) DecreaseTotalFraction(); return true;
					case KT_FractionInc:        if (CurrentDiff->Measures.size()) IncreaseTotalFraction(); return true;
					case KT_GridDec:            GridCellSize--; return true;
					case KT_GridInc:            GridCellSize++; return true;
					case KT_SwitchOffsetPrompt: OffsetPrompt.SwitchOpen(); return true;
					case KT_SwitchBPMPrompt:	BPMPrompt.SwitchOpen(); return true;
					}

					switch (key)
					{
					case 'S': SaveChart(); return true;
					case 'M': CurrentDiff->Measures[Measure].clear(); return true;
					case 'Q':
						if (Mode == Select)
							Mode = Normal;
						else if (Mode == Normal)
							Mode = Hold;
						else
							Mode = Select;
						return true;
					case 'T':
						InsertMeasure();
						return true;
					case 'X':
						if (Measure + 1 < CurrentDiff->Measures.size())
						{
							Measure++;
							CurrentFraction = 0;
						}
						return true;
					case 'Z':
						if (Measure > 0)
						{
							Measure--;
							CurrentFraction = 0;
						}
						return true;
					case 'G': GridEnabled = !GridEnabled; return true;
					}
				}
			}
			else // mouse input
			{
				if (EditScreenState == Editing)
				{
					if (code == KE_PRESS)
						OnMousePress(tkey);
					if (code == KE_RELEASE)
						OnMouseRelease(tkey);
				}
			}

			return false;
		}

		void ScreenEdit::RunGhostObject()
		{
			GhostObject.SetPositionY(YLock);

			if (!GridEnabled)
			{
				GhostObject.SetPositionX(WindowFrame.GetRelativeMPos().x);
			}
			else
			{
				auto CellSize = ScreenWidth / GridCellSize;
				auto Mod = (int)(WindowFrame.GetRelativeMPos().x - ScreenDifference) % CellSize;
				GhostObject.SetPositionX((int)WindowFrame.GetRelativeMPos().x - Mod);
			}

			if ((GhostObject.GetPosition().x - ScreenDifference) > PlayfieldWidth)
				GhostObject.SetPositionX(PlayfieldWidth + ScreenDifference);
			if ((GhostObject.GetPosition().x - ScreenDifference) < 0)
				GhostObject.SetPositionX(ScreenDifference);

			if (Mode != Select)
				GhostObject.Render();
		}

		void ScreenEdit::DrawInformation()
		{
			std::stringstream info;
			info << "Beat: " << (float)Measure * MySong->MeasureLength + MySong->MeasureLength * ((float)CurrentFraction) / Fracs[CurrentTotalFraction];
			if (CurrentDiff->Measures.size())
				info << "\nMeasureFraction: " << Fracs[CurrentTotalFraction];
			info << "\nMode:    ";
			if (Mode == Normal)
				info << "Normal";
			else if (Mode == Hold)
				info << "Hold";
			else
				info << "Null";
			if (GridEnabled)
				info << "\nGrid Enabled (size " << ScreenWidth / GridCellSize << ")";
			EditInfo.Render(info.str(), Vec2(512, 600));
		}

		void ScreenEdit::CalculateVerticalLock()
		{
			if (CurrentDiff->Measures.size())
			{
				float frac = ((float)CurrentFraction / Fracs[CurrentTotalFraction]);
				if (!(Measure % 2))
					YLock = frac * (float)PlayfieldHeight;
				else
					YLock = PlayfieldHeight - frac * (float)PlayfieldHeight;

				YLock += ScreenOffset;
			}
		}

		bool ScreenEdit::Run(double delta)
		{
			// we're playing the song? run the game
			if (EditScreenState == Playing)
			{
				if (!Music->IsPlaying())
					startMusic();
				ScreenGameplay::Run(delta);
			}
			else // editing the song? run the editor
			{
				CalculateVerticalLock();
				RenderObjects(delta, false); // No need to draw playable elements since that's what WE do!

				if (CurrentDiff->Measures.size())
				{
					double Ratio = ((float)CurrentFraction / Fracs[CurrentTotalFraction]);;
					Barline.Run(delta, Ratio);
					if (Measure > 0)
						DrawVector(CurrentDiff->Measures.at(Measure - 1), delta);
					DrawVector(CurrentDiff->Measures[Measure], delta);
				}

				RunGhostObject();
				DrawInformation();
				OffsetPrompt.Render();
				BPMPrompt.Render();
			}

			return Running;
		}

		void ScreenEdit::IncreaseTotalFraction()
		{
			double Frac = (CurrentFraction / Fracs[CurrentTotalFraction]);
			int ClosestFrac;

			CurrentTotalFraction += 1;

			if (CurrentTotalFraction >= sizeof(Fracs) / sizeof(double))
				CurrentTotalFraction -= 1;

			ClosestFrac = Frac * Fracs[CurrentTotalFraction];

			// Normalize.
			CurrentFraction = ClosestFrac;
		}

		void ScreenEdit::DecreaseTotalFraction()
		{
			double Frac = (CurrentFraction / Fracs[CurrentTotalFraction]);
			int ClosestFrac;

			CurrentTotalFraction -= 1;

			// Don't forget it's unsigned, and as such it could overflow instead.
			if (CurrentTotalFraction >= sizeof(Fracs) / sizeof(double))
				CurrentTotalFraction = 0;

			ClosestFrac = Frac * Fracs[CurrentTotalFraction];

			// Normalize.
			CurrentFraction = ClosestFrac;
		}

		GameObject &ScreenEdit::GetObject()
		{
			for (std::vector<GameObject>::iterator i = CurrentDiff->Measures.at(Measure).begin();
			i != CurrentDiff->Measures.at(Measure).end();
				i++)
			{
				if (i->GetFraction() * 192 == (CurrentFraction / Fracs[CurrentTotalFraction]) * 192)
					return *i;
			}
			CurrentDiff->Measures.at(Measure).push_back(GameObject());
			return CurrentDiff->Measures.at(Measure).back();
		}
	}
}