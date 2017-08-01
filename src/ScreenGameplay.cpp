#include "pch.h"

#include "GameGlobal.h"
#include "GameState.h"
#include "ScreenGameplay.h"
#include "ScreenEvaluation.h"
#include "GameWindow.h"
#include "ImageLoader.h"
#include "Audio.h"

#define ComboSizeX 24
#define ComboSizeY 48


namespace Game {
	namespace dotcur {
		ScreenGameplay::ScreenGameplay() :
			Screen("ScreenGameplay", false),
			Barline(this)
		{
			Running = true;
			IsAutoplaying = false;
			ShouldChangeScreenAtEnd = true;
			SongInfo.LoadSkinFontImage("font.tga", Vec2(6, 15), Vec2(8, 16), Vec2(6, 15), 0);
			MyFont.LoadSkinFontImage("font-combo.tga", Vec2(8, 10), Vec2(8, 10), Vec2(32, 40), 48);
			Music = NULL;
			FailEnabled = true;
			TappingMode = false;
			EditMode = false;
			IsPaused = false;
			LeadInTime = 0;
		}

		void ScreenGameplay::RemoveTrash()
		{
			NotesHeld.clear();
			AnimateOnly.clear();
			NotesInMeasure.clear();
		}

		void ScreenGameplay::Cleanup()
		{
			// Deleting the song's notes is ScreenSelectMusic's (or fileman's) job.
			if (Music != NULL)
			{
				delete Music;
				Music = NULL;
			}
			WindowFrame.SetVisibleCursor(true);
		}

		Vec2 ScreenGameplay::GetScreenOffset(float Alignment)
		{
			float outx;
			float outy;

			outx = (WindowFrame.GetMatrixSize().x*Alignment);
			outy = (WindowFrame.GetMatrixSize().y*Alignment);

			return Vec2(outx, outy);
		}

		void ScreenGameplay::StoreEvaluation(Judgment Eval)
		{
			if (Eval > Bad || Eval == OK)
				Combo++;

			switch (Eval)
			{
			case J_EXCELLENT:
				Evaluation.NumExcellents++;
				Evaluation.dpScore += 2;
				Evaluation.dpScoreSquare += Evaluation.dpScore;
				break;
			case J_PERFECT:
				Evaluation.NumPerfects++;
				Evaluation.dpScore += 1;
				Evaluation.dpScoreSquare += Evaluation.dpScore * 0.5;
				break;
			case J_GREAT:
				Evaluation.NumGreats++;
				Evaluation.dpScoreSquare += std::max(1.0, Evaluation.dpScore) * 0.1;
				break;
			case Bad:
				Evaluation.NumBads++;
				Evaluation.dpScore = std::max(Evaluation.dpScore - 4, 0.0);
				Combo = 0;
				break;
			case Miss:
				Evaluation.NumMisses++;
				Evaluation.dpScore = std::max(Evaluation.dpScore - 8, 0.0);
				Combo = 0;
				break;
			case NG:
				Evaluation.NumNG++;
				Combo = 0;
				break;
			case OK:
				Evaluation.NumOK++;
				break;
			case None:
				break;
			}
			Evaluation.MaxCombo = std::max(Evaluation.MaxCombo, Combo);
		}

		void ScreenGameplay::MainThreadInitialization()
		{
			Cursor.SetImage(GameState::GetInstance().GetSkinImage("cursor.png"));
			Barline.SetImage(GameState::GetInstance().GetSkinImage("barline.png"));
			MarkerA.SetImage(GameState::GetInstance().GetSkinImage("barline_marker.png"));
			MarkerB.SetImage(GameState::GetInstance().GetSkinImage("barline_marker.png"));
			GameplayObjectImage = GameState::GetInstance().GetSkinImage("hitcircle.png");

			Texture* BackgroundImage = ImageLoader::Load(MySong->SongDirectory / MySong->BackgroundFilename);

			if (BackgroundImage)
				Background.SetImage(BackgroundImage);
			else
				Background.SetImage(GameState::GetInstance().GetSkinImage(Configuration::GetSkinConfigs("DefaultGameplayBackground")));

			Background.AffectedByLightning = true;
			Background.Alpha = 0.8f;

			if (Background.GetImage())
			{
				float SizeRatio = 768 / Background.GetHeight();
				Background.SetScale(SizeRatio);
				Background.Centered = true;
				Background.SetPosition(ScreenWidth / 2, ScreenHeight / 2);
			}

			MarkerB.Centered = true;
			MarkerA.Centered = true;

			MarkerA.SetPosition(GetScreenOffset(0.5).x, MarkerA.GetHeight() / 2 - Barline.GetHeight());
			MarkerB.SetPosition(GetScreenOffset(0.5).x, ScreenHeight - MarkerB.GetHeight() / 2 + Barline.GetHeight() / 2);
			MarkerA.SetRotation(180);

			MarkerB.AffectedByLightning = MarkerA.AffectedByLightning = true;

			Lifebar.UpdateHealth();

			ReadySign.SetImage(GameState::GetInstance().GetSkinImage("ready.png"));
			ReadySign.SetPosition(0, ScreenHeight);
			ReadySign.Centered = true;
			ReadySign.AffectedByLightning = true;

			Cursor.SetSize(CursorSize);
			Cursor.Centered = CursorCentered;
			Cursor.AffectedByLightning = true;

			SongTime -= LeadInTime;

			// Start with lights off.
			if (ShouldChangeScreenAtEnd)
			{
			}//WindowFrame.SetLightMultiplier(0);
			else
			{
			} // WindowFrame.SetLightMultiplier(1.2f);
			WindowFrame.SetVisibleCursor(false);
		}

		void ScreenGameplay::ResetNotes()
		{
			BarlineRatios = CurrentDiff->BarlineRatios;
			NotesInMeasure.resize(CurrentDiff->Measures.size());
			for (auto i = 0U; i < CurrentDiff->Measures.size(); i++)
			{
				NotesInMeasure[i] = CurrentDiff->Measures[i];
				for (std::vector<GameObject>::iterator k = NotesInMeasure[i].begin(); k != NotesInMeasure[i].end(); k++)
				{
					if (k->GetPosition().x == 0)
						k = NotesInMeasure[i].erase(k);
					else
						Evaluation.totalNotes++;

					if (k == NotesInMeasure[i].end()) break;
				}
			}
		}

		void ScreenGameplay::LoadResources()
		{
			const char* SkinFiles[] =
			{
				"cursor.png",
				"Barline.png",
				"barline_marker.png",
				"hitcircle.png",
				"Ready.png"
			};

			ImageLoader::LoadFromManifest(SkinFiles, 3, GameState::GetInstance().GetSkinPrefix());

			memset(&Evaluation, 0, sizeof(Evaluation));

			Measure = 0;
			Combo = 0;

			if (ShouldChangeScreenAtEnd)
				Barline.Init(CurrentDiff->Offset + MySong->LeadInTime);
			else
			{
				Barline.Init(0); // edit mode
				Barline.Alpha = 1;
			}

			Lifebar.UpdateHealth();

			if (CurrentDiff->Timing.size())
				MeasureTime = (60 * MySong->MeasureLength / CurrentDiff->Timing[0].Value);
			else
				MeasureTime = 0;

			// We might be retrying- in that case we should probably clean up.
			RemoveTrash();

			ResetNotes();

			if (!Music)
			{
				Music = new AudioStream();

				if (!Music || !Music->Open(MySong->SongDirectory / MySong->SongFilename))
				{
					throw std::runtime_error((Utility::Format("couldn't open song %s", MySong->SongFilename.c_str()).c_str()));
				}

				seekTime(0);
			}

			MeasureRatio = 0;
			RatioPerSecond = 0;

			if (Music)
			{
				if (ShouldChangeScreenAtEnd)
					LeadInTime = MySong->LeadInTime;
			}

			CursorRotospeed = Configuration::GetSkinConfigf("RotationSpeed", "Cursor");
			CursorCentered = Configuration::GetSkinConfigf("Centered", "Cursor") != 0;
			CursorZooming = Configuration::GetSkinConfigf("Zooming", "Cursor") != 0;
			CursorSize = Configuration::GetSkinConfigf("Size", "CursorSize");

			if (CursorSize == 0)
				CursorSize = 60;
		}

		void ScreenGameplay::Init(dotcur::Song *OtherSong, uint32_t DifficultyIndex)
		{
			MySong = OtherSong;
			CurrentDiff = MySong->Difficulties[DifficultyIndex];
		}

		int32_t ScreenGameplay::GetMeasure()
		{
			return Measure;
		}

		/* Note stuff */

		void ScreenGameplay::RunVector(std::vector<GameObject>& Vec, float TimeDelta)
		{
			Judgment Val;

			if (LeadInTime > 0) // No running while leadin is up.
				return;

			// For each note in current measure
			for (std::vector<GameObject>::iterator i = Vec.begin();
			i != Vec.end();
				i++)
			{
				// Run the note.
				if ((Val = i->Run(TimeDelta, SongTime, IsAutoplaying)) != None)
				{
					Lifebar.HitJudgment(Val);
					aJudgment.ChangeJudgment(Val);
					StoreEvaluation(Val);

					// If it's a hold, keep running it until it's done. (Autoplay stuff.)
					if (i->IsHold() && Val > Miss)
					{
						NotesHeld.push_back(*i);
						i = Vec.erase(i); // These notes are off the measure. We'll handle them somewhere else.
					}
					break;
				}
			}
		}

		void ScreenGameplay::RunMeasure(float delta)
		{
			if (Measure < CurrentDiff->Measures.size())
			{
				RunVector(NotesInMeasure[Measure], delta);

				// For each note in PREVIOUS measure (when not in edit mode)
				if (Measure > 0 && !EditMode)
					RunVector(NotesInMeasure[Measure - 1], delta);

				// Run the ones in the NEXT measure.
				if (Measure + 1 < NotesInMeasure.size())
					RunVector(NotesInMeasure[Measure + 1], delta);
			}

			if (NotesHeld.size() > 0)
			{
				Judgment Val;
				for (std::vector<GameObject>::iterator i = NotesHeld.begin();
				i != NotesHeld.end();
					i++)
				{
					// See if something's going on with the hold.
					if ((Val = i->Run(delta, SongTime, IsAutoplaying)) != None)
					{
						// Judge accordingly..
						Lifebar.HitJudgment(Val);
						aJudgment.ChangeJudgment(Val);
						StoreEvaluation(Val);
						AnimateOnly.push_back(*i); // Animate this one.
						i = NotesHeld.erase(i);
						break;
					}
				}
			}
		}

		bool ScreenGameplay::HandleInput(int32_t key, KeyEventType code, bool isMouseInput)
		{
			if (Screen::HandleInput(key, code, isMouseInput))
				return true;

			/* Notes */
			if (Measure < CurrentDiff->Measures.size() && // if measure is playable
				(BindingsManager::TranslateKey(key) == KT_Select || BindingsManager::TranslateKey(key) == KT_SelectRight ||
					BindingsManager::TranslateKey(key) == KT_GameplayClick)// is mouse input and it's a mouse button..
				)
			{
				if (CursorZooming)
				{
					if (code == KE_PRESS)
						Cursor.SetScale(0.85f);
					else
						Cursor.SetScale(1);
				}

				// For all measure notes..
				if (!IsPaused)
				{
					do
					{
						if (Measure > 0 && JudgeVector(NotesInMeasure[Measure - 1], code, key))
							break;

						if (JudgeVector(NotesInMeasure[Measure], code, key))
							break;

						if (Measure + 1 < NotesInMeasure.size() && JudgeVector(NotesInMeasure[Measure + 1], code, key))
							break;
					} while (false);

					Judgment Val;
					Vec2 mpos = WindowFrame.GetRelativeMPos();
					// For all held notes...
					for (std::vector<GameObject>::iterator i = NotesHeld.begin();
					i != NotesHeld.end();
						i++)
					{
						// See if something's going on with the hold.
						if ((Val = i->Hit(SongTime, mpos, code != KE_RELEASE, IsAutoplaying, key)) != None)
						{
							// Judge accordingly..
							Lifebar.HitJudgment(Val);
							aJudgment.ChangeJudgment(Val);
							StoreEvaluation(Val);
							i = NotesHeld.erase(i); // Delete this object. The hold is done!
							break;
						}
					}
					return true;
				}
			}

			/* Functions */
			if (code == KE_PRESS)
			{
				switch (key)
				{
#ifndef NDEBUG
				case 'R':
					Cleanup();
					Init(MySong, 0);
					return true;
#endif
				case 'A': // Autoplay
					IsAutoplaying = !IsAutoplaying;
					break;
				case 'F':
					FailEnabled = !FailEnabled;
					break;
				case 'T':
					TappingMode = !TappingMode;
					break;
				case 'Q':
					Running = false;
					break;
				default:
					if (BindingsManager::TranslateKey(key) == KT_Escape)
					{
						/* TODO: pause */
						if (!IsPaused)
							stopMusic();
						else
						{
							Music->SeekTime(SongTime);
							startMusic();
						}
						IsPaused = !IsPaused;
					}
		}
				return true;
	}

			return false;
}

		void ScreenGameplay::seekTime(float Time)
		{
			Music->SeekTime(Time);
			SongTime = Time;
			MeasureRatio = 0;
		}

		void ScreenGameplay::startMusic()
		{
			Music->Play();
		}

		void ScreenGameplay::stopMusic()
		{
			Music->Stop();
		}

		/* TODO: Use measure ratios instead of song time for the barline. */
		bool ScreenGameplay::Run(double TimeDelta)
		{
			if (Music && LeadInTime <= 0)
			{
				SongDelta = Music->GetStreamedTime() - SongTime;

				SongTime += SongDelta;
			}

			float ScreenTime = Screen::GetScreenTime();
			if (ScreenTime > ScreenPauseTime || !ShouldChangeScreenAtEnd) // we're over the pause?
			{
				if (SongTime <= 0)
				{
					if (LeadInTime > 0)
					{
						LeadInTime -= TimeDelta;
						SongTime = -LeadInTime;
					}
					else
						startMusic();
				}

				if (Next) // We have a pending screen?
					return RunNested(TimeDelta); // use that instead.

				RunMeasure(SongDelta);

				if (LeadInTime)
					Barline.Run(TimeDelta, MeasureRatio);
				else
					Barline.Run(SongDelta, MeasureRatio);

				if (SongTime > CurrentDiff->Offset)
				{
					while (BarlineRatios.size() && BarlineRatios.at(0).Time <= SongTime)
					{
						RatioPerSecond = BarlineRatios.front().Value;
						BarlineRatios.erase(BarlineRatios.begin());
					}

					MeasureRatio += RatioPerSecond * SongDelta;

					if (SongDelta == 0 && !IsPaused)
					{
						if ((!Music || !Music->IsPlaying()))
							MeasureRatio += RatioPerSecond * TimeDelta;
					}

					if (MeasureRatio > 1.0f)
					{
						MeasureRatio -= 1.0f;
						Measure += 1;
					}
					Lifebar.Run(SongDelta);
					aJudgment.Run(TimeDelta);
				}
			}

			if (ShouldChangeScreenAtEnd)
			{
				float TotalTime = (CurrentDiff->Offset + MySong->LeadInTime + ScreenPauseTime);
				float X = GetScreenTime() / TotalTime;
				float xPos;

				if (X < 0.5)
					xPos = ((-2)*X*X + 2 * X) * ScreenWidth;
				else
					xPos = ScreenWidth - ((-2)*X*X + 2 * X) * ScreenWidth;

				ReadySign.SetPosition(xPos, ScreenHeight / 2);
				ReadySign.Alpha = 2 * ((-2)*X*X + 2 * X);

				// Lights
				/*float LightProgress = GetScreenTime() / 1.5;
				if (LightProgress <= 1)
					WindowFrame.SetLightMultiplier(LightProgress * 1.2);
					*/
			}
			else
				ReadySign.Alpha = 0;

			RenderObjects(TimeDelta);

			if (ShouldChangeScreenAtEnd && Measure >= CurrentDiff->Measures.size())
			{
				auto Eval = std::make_shared<ScreenEvaluation>();
				Eval->Init(Evaluation, MySong->SongAuthor, MySong->SongName);
				Next = Eval;
				Music->Stop();
			}

			// You died? Not editing? Failing is enabled?
			if (Lifebar.Health <= 0 && !EditMode && FailEnabled)
				Running = false; // It's over.

			return Running;
		}

		void ScreenGameplay::DrawVector(std::vector<GameObject>& Vec, float TimeDelta)
		{
			for (std::vector<GameObject>::reverse_iterator i = Vec.rbegin();
			i != Vec.rend();
				i++)
			{
				if (i->ShouldRemove())
					continue;

				i->SetImage(GameplayObjectImage);
				i->SetSize(CircleSize);
				i->Animate(TimeDelta, SongTime);
				i->ColorInvert = false;
				i->Render();
			}
		}

		bool ScreenGameplay::JudgeVector(std::vector<GameObject>& Vec, int code, int key)
		{
			Judgment Val;
			Vec2 mpos = WindowFrame.GetRelativeMPos();

			for (std::vector<GameObject>::iterator i = Vec.begin();
			i != Vec.end();
				i++)
			{
				// See if it's a hit.
				if ((Val = i->Hit(SongTime, TappingMode ? i->GetPosition() : mpos, code != KE_RELEASE, IsAutoplaying, key)) != None)
				{
					// Judge accordingly.
					Lifebar.HitJudgment(Val);
					aJudgment.ChangeJudgment(Val);
					StoreEvaluation(Val);

					// If it's a hold, keep running it until it's done.
					if (i->IsHold() && Val > Miss)
					{
						NotesHeld.push_back(*i);
						i = Vec.erase(i); // These notes are off the measure. We'll handle them somewhere else.
					}

					return true;
				}
			}
			return false;
		}

		void ScreenGameplay::RenderObjects(float TimeDelta, bool drawPlayable)
		{
			Vec2 mpos = WindowFrame.GetRelativeMPos();

			Cursor.SetPosition(mpos);

			Cursor.AddRotation(CursorRotospeed * TimeDelta);

			int Beat = MeasureRatio * MySong->MeasureLength;
			float Fraction = (float(MeasureRatio * MySong->MeasureLength) - Beat);
			Lifebar.SetScaleX(1.0 - 0.05 * Fraction);

			// Rendering ahead.

			if (IsPaused)
			{
				Background.Blue = Background.Red = Background.Green = 0.5;
			}
			else
				Background.Blue = Background.Red = Background.Green = 1;

			Background.Render();

			MarkerA.Render();
			MarkerB.Render();

			if (!EditMode)
				Lifebar.Render();

			if (drawPlayable)
			{
				DrawVector(NotesHeld, TimeDelta);
				DrawVector(AnimateOnly, TimeDelta);

				if (Measure > 0)
				{
					if (NotesInMeasure.size() && // there are measures and
						Measure - 1 < NotesInMeasure.size() && // the measure is within the range and
						NotesInMeasure.at(Measure - 1).size() > 0) // there are notes in this measure
					{
						DrawVector(NotesInMeasure[Measure - 1], TimeDelta);
					}
				}

				// Render current measure on front of the next!
				if (Measure + 1 < CurrentDiff->Measures.size())
				{
					if (NotesInMeasure.size() > Measure + 1 && NotesInMeasure.at(Measure + 1).size() > 0)
					{
						DrawVector(NotesInMeasure[Measure + 1], TimeDelta);
					}
				}

				if (Measure < CurrentDiff->Measures.size())
				{
					if (NotesInMeasure.size() > Measure && NotesInMeasure.at(Measure).size() > 0)
					{
						DrawVector(NotesInMeasure[Measure], TimeDelta);
					}
				}
			}

			Barline.Render();

			if (!EditMode)
				aJudgment.Render();

			// Combo rendering.
			std::stringstream str;
			str << Combo;

			float textX = GetScreenOffset(0.5).x - (str.str().length() * ComboSizeX / 2);
			MyFont.Render(str.str(), Vec2(textX, 0));

			std::stringstream str2;
			str2 << int32_t(1000000.0 * Evaluation.dpScoreSquare / (Evaluation.totalNotes * (Evaluation.totalNotes + 1)));
			textX = GetScreenOffset(0.5).x - (str2.str().length() * ComboSizeX / 2);
			MyFont.Render(str2.str(), Vec2(textX, 720));

			/* Lengthy information printing code goes here.*/
			std::stringstream info;

			if (IsAutoplaying)
				info << "Autoplay";

#ifndef NDEBUG
			info << "\nSongTime: " << SongTime << "\nPlaybackTime: ";
			if (Music)
				info << Music->GetPlayedTime();
			else
				info << "???";
			/*info << "\nStreamTime: ";
			if(Music)
				info << Music->GetStreamedTime();
			else
				info << "???";
				*/

			info << "\naudioFactor: " << MixerGetFactor();
			info << "\nSongDelta: " << SongDelta;
			info << "\nDevice Latency: " << (int)(MixerGetLatency() * 1000);
			/*
			if (Music)
				info << Music->GetStreamedTime() - Music->GetPlaybackTime();
			else
				info << "???";
				*/
			info << "\nScreenTime: " << GetScreenTime();
			info << "\nMeasureRatio: " << MeasureRatio;
			info << "\nMeasureRatioPerSecond: " << RatioPerSecond;
#endif
			if (TappingMode)
				info << "\nTapping mode";
			if (!FailEnabled)
				info << "\nFailing Disabled";

#ifdef NDEBUG
			if (EditMode)
#endif
				info << "\nMeasure: " << Measure;
			SongInfo.Render(info.str(), Vec2(0, 0));

			ReadySign.Render();

			Cursor.Render();
		}
	}
}