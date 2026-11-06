/*
 Copyright (c) 2013 yvt

 This file is part of OpenSpades.

 OpenSpades is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 OpenSpades is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with OpenSpades.  If not, see <http://www.gnu.org/licenses/>.

 */

#include <cctype>
#include <initializer_list>

#include "ChatTranslator.h"
#include "ChatWindow.h"
#include "Client.h"
#include "ClientUI.h"
#include "IFont.h"
#include "IRenderer.h"
#include "World.h"
#include <Core/Debug.h>
#include <Core/Exception.h>
#include <Core/Math.h>
#include <Core/Settings.h>

DEFINE_SPADES_SETTING(cg_chatHeight, "30");
DEFINE_SPADES_SETTING(cg_killfeedHeight, "26");

DEFINE_SPADES_SETTING(cg_chatFadeTime, "12");
DEFINE_SPADES_SETTING(cg_killfeedFadeTime, "6");

SPADES_SETTING(cg_stats);
SPADES_SETTING(cg_statsSmallFont);
SPADES_SETTING(cg_chatTranslate);

namespace spades {
	namespace client {

		namespace {
			/**
			 * Draws a small flag for a two-letter language code. Only flat rectangles are
			 * available, so the more elaborate designs are approximated.
			 */
			void DrawLanguageFlag(IRenderer& renderer, const std::string& lang, float x,
			                      float y, float w, float h, float fade) {
				auto fill = [&](float x0, float y0, float x1, float y1, const Vector4& c) {
					Vector4 col = MakeVector4(c.x, c.y, c.z, c.w * fade).Premultiplied();
					renderer.SetColorAlphaPremultiplied(col);
					renderer.DrawFilledRect(x0, y0, x1, y1);
				};
				auto bandsH = [&](std::initializer_list<Vector4> bands) {
					const float n = static_cast<float>(bands.size());
					int i = 0;
					for (const Vector4& c : bands) {
						fill(x, y + h * i / n, x + w, y + h * (i + 1) / n, c);
						++i;
					}
				};
				auto bandsV = [&](std::initializer_list<Vector4> bands) {
					const float n = static_cast<float>(bands.size());
					int i = 0;
					for (const Vector4& c : bands) {
						fill(x + w * i / n, y, x + w * (i + 1) / n, y + h, c);
						++i;
					}
				};
				auto nordicCross = [&](const Vector4& bg, const Vector4& cross,
				                       const Vector4* inner) {
					fill(x, y, x + w, y + h, bg);
					const float t = h * 0.28F;
					const float vx = x + w * 0.36F;
					const float cy = y + h * 0.5F;
					fill(vx - t * 0.5F, y, vx + t * 0.5F, y + h, cross);
					fill(x, cy - t * 0.5F, x + w, cy + t * 0.5F, cross);
					if (inner) {
						const float ti = t * 0.45F;
						fill(vx - ti * 0.5F, y, vx + ti * 0.5F, y + h, *inner);
						fill(x, cy - ti * 0.5F, x + w, cy + ti * 0.5F, *inner);
					}
				};

				const Vector4 white = MakeVector4(1.00F, 1.00F, 1.00F, 0.95F);
				const Vector4 red = MakeVector4(0.82F, 0.11F, 0.14F, 0.95F);
				const Vector4 gold = MakeVector4(0.98F, 0.78F, 0.06F, 0.95F);
				const Vector4 blue = MakeVector4(0.05F, 0.16F, 0.55F, 0.95F);
				const Vector4 black = MakeVector4(0.07F, 0.07F, 0.08F, 0.95F);
				const Vector4 green = MakeVector4(0.00F, 0.45F, 0.22F, 0.95F);

				const float cx = x + w * 0.5F;
				const float cy = y + h * 0.5F;

				if (lang == "es") {
					bandsH({ red, gold, red });
					fill(x + 3.0F, cy - 2.0F, x + 5.0F, cy + 1.0F,
					     MakeVector4(0.72F, 0.15F, 0.12F, 0.90F));
				} else if (lang == "en") {
					// Union Jack approximation: the diagonals cannot be drawn with rectangles.
					fill(x, y, x + w, y + h, MakeVector4(0.05F, 0.13F, 0.45F, 0.95F));
					fill(cx - w * 0.11F, y, cx + w * 0.11F, y + h, white);
					fill(x, cy - h * 0.20F, x + w, cy + h * 0.20F, white);
					fill(cx - w * 0.06F, y, cx + w * 0.06F, y + h, red);
					fill(x, cy - h * 0.10F, x + w, cy + h * 0.10F, red);
				} else if (lang == "fr") {
					bandsV({ blue, white, red });
				} else if (lang == "it") {
					bandsV({ green, white, red });
				} else if (lang == "de") {
					bandsH({ black, red, gold });
				} else if (lang == "nl") {
					bandsH({ red, white, blue });
				} else if (lang == "ru") {
					bandsH({ white, blue, red });
				} else if (lang == "pl") {
					bandsH({ white, red });
				} else if (lang == "id") {
					bandsH({ red, white });
				} else if (lang == "ro") {
					bandsV({ blue, gold, red });
				} else if (lang == "hu") {
					bandsH({ red, white, green });
				} else if (lang == "ua") {
					bandsH({ blue, gold });
				} else if (lang == "bg") {
					bandsH({ white, green, red });
				} else if (lang == "cs") {
					bandsH({ white, red });
					fill(x, y, x + w * 0.45F, y + h, MakeVector4(0.04F, 0.20F, 0.50F, 0.95F));
				} else if (lang == "tr") {
					fill(x, y, x + w, y + h, red);
					// Crescent: a white disc with a red one biting into it.
					fill(x + 3.0F, cy - 3.5F, x + 9.0F, cy + 3.5F, white);
					fill(x + 5.0F, cy - 3.0F, x + 9.0F, cy + 3.0F, red);
					fill(x + 10.0F, cy - 1.5F, x + 13.0F, cy + 1.5F, white);
				} else if (lang == "pt") {
					fill(x, y, x + w * 0.40F, y + h, green);
					fill(x + w * 0.40F, y, x + w, y + h, red);
					fill(x + w * 0.40F - 2.0F, cy - 2.0F, x + w * 0.40F + 2.0F, cy + 2.0F, gold);
				} else if (lang == "ja") {
					fill(x, y, x + w, y + h, white);
					fill(cx - w * 0.22F, cy - h * 0.28F, cx + w * 0.22F, cy + h * 0.28F, red);
				} else if (lang == "zh") {
					fill(x, y, x + w, y + h, red);
					fill(x + 2.0F, y + 1.5F, x + 5.0F, y + 4.5F, gold);
					fill(x + 6.0F, y + 2.0F, x + 8.0F, y + 4.0F, gold);
				} else if (lang == "ko") {
					fill(x, y, x + w, y + h, white);
					fill(cx - 3.0F, cy - 3.0F, cx + 3.0F, cy, red);
					fill(cx - 3.0F, cy, cx + 3.0F, cy + 3.0F, blue);
				} else if (lang == "vi") {
					fill(x, y, x + w, y + h, red);
					fill(cx - 2.0F, cy - 2.0F, cx + 2.0F, cy + 2.0F, gold);
				} else if (lang == "sv") {
					nordicCross(blue, gold, nullptr);
				} else if (lang == "fi") {
					nordicCross(white, blue, nullptr);
				} else if (lang == "da") {
					nordicCross(red, white, nullptr);
				} else if (lang == "no" || lang == "nb" || lang == "nn") {
					nordicCross(red, white, &blue);
				} else if (lang == "el") {
					bandsH({ blue, white, blue });
				} else if (lang == "he") {
					fill(x, y, x + w, y + h, white);
					fill(x, y, x + w, y + 2.0F, blue);
					fill(x, y + h - 2.0F, x + w, y + h, blue);
				} else if (lang == "ar") {
					bandsH({ green, white, black });
				} else if (lang == "th") {
					bandsH({ red, white, blue });
				} else {
					// Unknown language: neutral badge with a light "translate" bar.
					fill(x, y, x + w, y + h, MakeVector4(0.28F, 0.30F, 0.34F, 0.92F));
					fill(x + 2.0F, cy - 1.0F, x + w - 2.0F, cy + 1.0F,
					     MakeVector4(0.90F, 0.90F, 0.92F, 0.70F));
				}

				renderer.SetColorAlphaPremultiplied(
				  MakeVector4(0.12F, 0.12F, 0.16F, 0.85F * fade).Premultiplied());
				renderer.DrawOutlinedRect(x, y, x + w, y + h);
			}
		} // namespace

		std::string ChatWindow::TranslateMarker(const std::string& lang) {
			std::string code = "xx";
			if (lang.size() >= 2) {
				code.clear();
				for (size_t i = 0; i < 2; i++) {
					char c = lang[i];
					if (c >= 'A' && c <= 'Z')
						c = static_cast<char>(c - 'A' + 'a');
					if (c < 'a' || c > 'z') {
						code = "xx";
						break;
					}
					code += c;
				}
			}
			return std::string(1, MsgTranslateIcon) + code;
		}

		ChatWindow::ChatWindow(Client* c, IFont* fnt, bool killfeed)
		    : client(c), renderer(c->GetRenderer()), font(fnt), killfeed(killfeed) {
			firstY = 0.0F;
			killImages.push_back(renderer.RegisterImage("Gfx/Killfeed/a-Rifle.png"));
			killImages.push_back(renderer.RegisterImage("Gfx/Killfeed/b-SMG.png"));
			killImages.push_back(renderer.RegisterImage("Gfx/Killfeed/c-Shotgun.png"));
			killImages.push_back(renderer.RegisterImage("Gfx/Killfeed/d-Headshot.png"));
			killImages.push_back(renderer.RegisterImage("Gfx/Killfeed/e-Melee.png"));
			killImages.push_back(renderer.RegisterImage("Gfx/Killfeed/f-Grenade.png"));
			killImages.push_back(renderer.RegisterImage("Gfx/Killfeed/g-Falling.png"));
			killImages.push_back(renderer.RegisterImage("Gfx/Killfeed/h-Teamchange.png"));
			killImages.push_back(renderer.RegisterImage("Gfx/Killfeed/i-Classchange.png"));
			killImages.push_back(renderer.RegisterImage("Gfx/Killfeed/j-Airborne.png"));
			killImages.push_back(renderer.RegisterImage("Gfx/Killfeed/k-Noscope.png"));
			killImages.push_back(renderer.RegisterImage("Gfx/Killfeed/l-Domination.png"));
			killImages.push_back(renderer.RegisterImage("Gfx/Killfeed/m-Revenge.png"));
		}

		ChatWindow::~ChatWindow() {}

		float ChatWindow::GetWidth() { return renderer.ScreenWidth() * 0.5F; }

		float ChatWindow::GetHeight() {
			float prop = killfeed ? (float)cg_killfeedHeight : (float)cg_chatHeight;
			return renderer.ScreenHeight() * prop * 0.01F;
		}

		float ChatWindow::GetLineHeight() { return 20.0F; }

		std::string ChatWindow::KillImage(int kt, int weapon) {
			std::string tmp = "xx";
			tmp[0] = MsgImage;
			switch (kt) {
				case KillTypeWeapon:
					switch (weapon) {
						case RIFLE_WEAPON:
						case SMG_WEAPON:
						case SHOTGUN_WEAPON:
							tmp[1] = 'a' + weapon;
							break;
						default: return "";
					}
					break;
				case KillTypeHeadshot:
				case KillTypeMelee:
				case KillTypeGrenade:
				case KillTypeFall:
				case KillTypeTeamChange:
				case KillTypeClassChange:
				case 7:  // airborne kill
				case 8:  // noscope kill
				case 9:  // domination kill
				case 10: // revenge kill
					tmp[1] = 'a' + 2 + kt;
					break;
				default: return "";
			}
			return tmp;
		}
		IImage* ChatWindow::GetKillImage(char index) {
			int real = index - 'a';
			if (real >= 0 && real < (int)killImages.size())
				return killImages[real].GetPointerOrNull();
			return NULL;
		}

		float ChatWindow::CalculateMessageHeight(const std::string& msg) {
			float lh = GetLineHeight();
			if (killfeed)
				return lh;
			float h = lh;
			for (char c : msg) {
				if (c == '\n')
					h += lh;
			}
			return h;
		}

		uint64_t ChatWindow::AddMessage(const std::string& msg, float fadeTime) {
			SPADES_MARK_FUNCTION();

			static uint64_t s_nextMessageId = 1;
			uint64_t id = s_nextMessageId++;

			float entryH = CalculateMessageHeight(msg);
			float fadeOut = killfeed ? 0.5F : 1.0F;

			if (fadeTime < 0.0F)
				fadeTime = killfeed ? cg_killfeedFadeTime : cg_chatFadeTime;

			entries.push_front(ChatEntry(id, msg, entryH, fadeTime + fadeOut));

			firstY -= entryH;
			return id;
		}

		void ChatWindow::UpdateMessage(uint64_t id, const std::string& newMsg) {
			for (auto& ent : entries) {
				if (ent.id == id) {
					float oldH = ent.height;
					ent.msg = newMsg;
					float newH = CalculateMessageHeight(newMsg);
					if (newH != oldH) {
						ent.height = newH;
						firstY -= (newH - oldH);
					}
					return;
				}
			}
		}

		std::string ChatWindow::ColoredMessage(const std::string& msg, char c) {
			SPADES_MARK_FUNCTION_DEBUG();
			std::string s;
			s += c;
			s += msg;
			s += MsgColorRestore;
			return s;
		}

		std::string ChatWindow::TeamColorMessage(const std::string& msg, int team) {
			SPADES_MARK_FUNCTION_DEBUG();
			switch (team) {
				case 0: return ColoredMessage(msg, MsgColorTeam1);
				case 1: return ColoredMessage(msg, MsgColorTeam2);
				case 2: return ColoredMessage(msg, MsgColorTeam3);
				default: return msg;
			}
		}

		Vector4 ChatWindow::GetColor(char c) {
			World* w = client ? client->GetWorld() : NULL;
			switch (c) {
				case MsgColorTeam1:
					return w ? ConvertColorRGBA(w->GetTeamColor(0)) : MakeVector4(0, 1, 0, 1);
				case MsgColorTeam2:
					return w ? ConvertColorRGBA(w->GetTeamColor(1)) : MakeVector4(0, 0, 1, 1);
				case MsgColorTeam3:
					return w ? ConvertColorRGBA(w->GetTeamColor(2)) : MakeVector4(1, 1, 0, 1);
				case MsgColorRed: return MakeVector4(1, 0.25, 0.25, 1);
				case MsgColorGreen: return MakeVector4(0, 1, 0, 1);
				case MsgColorGray: return MakeVector4(0.8F, 0.8F, 0.8F, 1);
				case MsgColorYellow: return MakeVector4(1, 1, 0.5, 1);
				default: return MakeVector4(1, 1, 1, 1);
			}
		}

		void ChatWindow::Update(float dt) {
			if (cg_chatTranslate && !killfeed && ChatTranslator::GetInstance().HasCompletedTranslations()) {
				auto completed = ChatTranslator::GetInstance().PopCompletedTranslations();
				for (const auto& item : completed) {
					UpdateMessage(item.msgId, item.fullTranslatedText);
					if (client && client->GetUI()) {
						client->GetUI()->UpdateChatLogTranslation(item.originalMsg,
						                                          item.translatedMsg,
						                                          item.sourceLang);
					}
				}
			}

			if (firstY < 0.0F) {
				firstY += dt * std::max(100.0F, -firstY);
				if (firstY > 0.0F)
					firstY = 0.0F;
			}

			float height = GetHeight();
			float y = firstY;

			for (auto it = entries.begin(); it != entries.end();) {
				ChatEntry& ent = *it;

				// Stacking direction doesn't matter here: the chat is the mirror
				// image of the killfeed, so the same conditions apply to both.
				if (y + ent.height > height) {
					// fade out, then remove
					ent.fade -= dt * 4.0F;
					if (ent.fade < 0.0F) {
						it = entries.erase(it);
						continue;
					}
				} else if (y + ent.height > 0.0F) {
					// fade in
					ent.fade = std::min(ent.fade + dt * 4.0F, 1.0F);
				}

				ent.timeFade -= dt;
				if (ent.timeFade < 0.0F) {
					// Collapse the expired line so the others slide into its place,
					// then remove it once it has no height left.
					ent.height = std::max(ent.height - dt * 100.0F, 0.0F);
					if (ent.height <= 0.0F) {
						it = entries.erase(it);
						continue;
					}
				}

				y += ent.height;
				++it;
			}
		}

		void ChatWindow::Draw() {
			SPADES_MARK_FUNCTION();

			float sh = renderer.ScreenHeight();

			float winH = GetHeight();
			float winX = 8.0F;
			float winY = killfeed ? 8.0F : (sh - 64.0F) - winH;
			float lh = GetLineHeight();
			float y = killfeed ? firstY : winH - firstY;

			if (killfeed) {
				bool demoMode = client->IsDemoMode();
				const int statsMode = cg_stats;
				if ((statsMode == 2 || (statsMode >= 3 && client->IsScoreboardVisible())) && !demoMode)
					winY += cg_statsSmallFont ? 10.0F : 20.0F;
			}

			Vector4 shadowColor = { 0, 0, 0, 0.85F };

			std::string ch = "aaaaaa"; // let's not make a new object for each character.
			// note: UTF-8's longest character is 6 bytes

			std::list<ChatEntry>::iterator it;
			for (it = entries.begin(); it != entries.end(); ++it) {
				ChatEntry& ent = *it;

				// top of this entry: the chat stacks upwards, the killfeed downwards.
				float entryY = killfeed ? y : y - ent.height;

				const auto& msg = ent.msg;
				Vector4 color = GetColor(MsgColorRestore);

				float curPosX = winX;
				float tx = 0.0F, ty = entryY;
				float fade = ent.fade;

				float fadeOut = killfeed ? 0.5F : 1.0F;
				if (ent.timeFade < fadeOut)
					fade *= (ent.timeFade / fadeOut);

				if (fade < 0.01F)
					goto endDrawLine; // Skip rendering invisible messages

				shadowColor.w = (killfeed ? 0.45F : 0.85F) * fade;
				color.w = fade;

				// animation for killfeed
				if (killfeed) {
					float easeOutFactor = 1.0F - powf(1.0F - fade, 3.0F);
					curPosX -= floorf(300.0F * (1.0F - easeOutFactor));
				}

				for (size_t i = 0; i < msg.size(); i++) {
					if (msg[i] == MsgTranslateIcon) {
						// Two lowercase letters right after the marker name the language the
						// message was written in, so the flag shows the source language and not
						// the one it was translated to.
						std::string lang = "xx";
						if (i + 2 < msg.size() && msg[i + 1] >= 'a' && msg[i + 1] <= 'z' &&
						    msg[i + 2] >= 'a' && msg[i + 2] <= 'z') {
							lang.assign(msg, i + 1, 2);
							i += 2;
						}

						float flagW = 16.0F;
						float flagH = 11.0F;
						float flagX = tx + curPosX;
						float flagY = ty + winY + (lh - flagH) * 0.5F;

						// Shadow
						Vector4 shadow = MakeVector4(0.0F, 0.0F, 0.0F, 0.65F * fade);
						renderer.SetColorAlphaPremultiplied(shadow.Premultiplied());
						renderer.DrawFilledRect(flagX + 1.0F, flagY + 1.0F, flagX + flagW + 1.0F, flagY + flagH + 1.0F);

						DrawLanguageFlag(renderer, lang, flagX, flagY, flagW, flagH, fade);

						tx += flagW + 5.0F;
					} else if (msg[i] == MsgDeadIcon) {
						// Clean vector dead skull icon 💀
						float iconW = 13.0F;
						float iconH = 12.0F;
						float iconX = tx + curPosX;
						float iconY = ty + winY + (lh - iconH) * 0.5F;

						// Skull shadow
						Vector4 skullShadow = MakeVector4(0.0F, 0.0F, 0.0F, 0.70F * fade).Premultiplied();
						renderer.SetColorAlphaPremultiplied(skullShadow);
						renderer.DrawFilledRect(iconX + 1.0F, iconY + 1.0F, iconX + iconW + 1.0F, iconY + iconH + 1.0F);

						// Bone white cranium and jaw
						Vector4 skullCol = MakeVector4(0.92F, 0.92F, 0.95F, 0.95F * fade).Premultiplied();
						Vector4 eyeCol = MakeVector4(0.12F, 0.12F, 0.15F, 0.95F * fade).Premultiplied();

						// Cranium
						renderer.SetColorAlphaPremultiplied(skullCol);
						renderer.DrawFilledRect(iconX + 1.5F, iconY, iconX + iconW - 1.5F, iconY + 7.5F);
						renderer.DrawFilledRect(iconX, iconY + 1.5F, iconX + iconW, iconY + 6.5F);

						// Jaw
						renderer.DrawFilledRect(iconX + 3.0F, iconY + 7.5F, iconX + iconW - 3.0F, iconY + 11.0F);

						// Eye sockets (dark)
						renderer.SetColorAlphaPremultiplied(eyeCol);
						renderer.DrawFilledRect(iconX + 2.5F, iconY + 2.5F, iconX + 5.0F, iconY + 5.0F);
						renderer.DrawFilledRect(iconX + iconW - 5.0F, iconY + 2.5F, iconX + iconW - 2.5F, iconY + 5.0F);

						// Nose cavity slit
						renderer.DrawFilledRect(iconX + (iconW * 0.5F) - 0.75F, iconY + 5.5F, iconX + (iconW * 0.5F) + 0.75F, iconY + 7.0F);

						// Teeth slits
						renderer.DrawFilledRect(iconX + 4.5F, iconY + 8.5F, iconX + 5.5F, iconY + 11.0F);
						renderer.DrawFilledRect(iconX + iconW - 5.5F, iconY + 8.5F, iconX + iconW - 4.5F, iconY + 11.0F);

						tx += iconW + 4.0F;
					} else if (msg[i] == MsgSpecIcon) {
						// Clean vector spectator eye icon 👁️
						float iconW = 15.0F;
						float iconH = 10.0F;
						float iconX = tx + curPosX;
						float iconY = ty + winY + (lh - iconH) * 0.5F;

						// Shadow
						Vector4 eyeShadow = MakeVector4(0.0F, 0.0F, 0.0F, 0.70F * fade).Premultiplied();
						renderer.SetColorAlphaPremultiplied(eyeShadow);
						renderer.DrawFilledRect(iconX + 1.0F, iconY + 1.0F, iconX + iconW + 1.0F, iconY + iconH + 1.0F);

						// Outer eye contour / sclera (pure clean white with slight soft cyan tint)
						Vector4 scleraCol = MakeVector4(0.95F, 0.98F, 1.0F, 0.95F * fade).Premultiplied();
						Vector4 irisCol = MakeVector4(0.20F, 0.75F, 1.0F, 0.95F * fade).Premultiplied(); // Azure cyan iris
						Vector4 pupilCol = MakeVector4(0.08F, 0.10F, 0.15F, 0.95F * fade).Premultiplied(); // Dark pupil
						Vector4 glintCol = MakeVector4(1.0F, 1.0F, 1.0F, 0.95F * fade).Premultiplied(); // Specular highlight

						// Almond eye sclera (tapered shape)
						renderer.SetColorAlphaPremultiplied(scleraCol);
						renderer.DrawFilledRect(iconX + 2.0F, iconY, iconX + iconW - 2.0F, iconY + iconH);
						renderer.DrawFilledRect(iconX, iconY + 2.5F, iconX + iconW, iconY + iconH - 2.5F);
						renderer.DrawFilledRect(iconX + 1.0F, iconY + 1.2F, iconX + iconW - 1.0F, iconY + iconH - 1.2F);

						// Iris (centered circle/rect)
						float irisW = 6.0F;
						float irisH = 6.0F;
						float irisX = iconX + (iconW - irisW) * 0.5F;
						float irisY = iconY + (iconH - irisH) * 0.5F;
						renderer.SetColorAlphaPremultiplied(irisCol);
						renderer.DrawFilledRect(irisX, irisY, irisX + irisW, irisY + irisH);

						// Pupil (centered)
						float pupilW = 3.0F;
						float pupilH = 3.0F;
						float pupilX = iconX + (iconW - pupilW) * 0.5F;
						float pupilY = iconY + (iconH - pupilH) * 0.5F;
						renderer.SetColorAlphaPremultiplied(pupilCol);
						renderer.DrawFilledRect(pupilX, pupilY, pupilX + pupilW, pupilY + pupilH);

						// Specular glint
						renderer.SetColorAlphaPremultiplied(glintCol);
						renderer.DrawFilledRect(pupilX + 1.5F, pupilY - 0.5F, pupilX + 2.7F, pupilY + 0.7F);

						// Thin dark eyelid / border outlines
						Vector4 outlineCol = MakeVector4(0.15F, 0.20F, 0.30F, 0.85F * fade).Premultiplied();
						renderer.SetColorAlphaPremultiplied(outlineCol);
						renderer.DrawFilledRect(iconX + 2.0F, iconY, iconX + iconW - 2.0F, iconY + 1.0F); // top lid
						renderer.DrawFilledRect(iconX + 2.0F, iconY + iconH - 1.0F, iconX + iconW - 2.0F, iconY + iconH); // bottom lid
						renderer.DrawFilledRect(iconX, iconY + 2.0F, iconX + 1.0F, iconY + iconH - 2.0F); // left corner
						renderer.DrawFilledRect(iconX + iconW - 1.0F, iconY + 2.0F, iconX + iconW, iconY + iconH - 2.0F); // right corner

						tx += iconW + 4.0F;
					} else if ((msg[i] == '\r' || msg[i] == '\n') && !killfeed) {
						// new line
						tx = 0.0F;
						ty += lh;
					} else if (msg[i] >= 1 && msg[i] <= MsgColorMax) {
						if (msg[i] == MsgImage) {
							IImage* img = NULL;
							if (i + 1 < msg.size() && (img = GetKillImage(msg[i + 1]))) {
								Vector4 colorP = color;
								colorP.x *= colorP.w;
								colorP.y *= colorP.w;
								colorP.z *= colorP.w;
								renderer.SetColorAlphaPremultiplied(colorP);
								renderer.DrawImage(img, MakeVector2(tx + curPosX + 1, ty + winY).Floor());
								tx += (int)roundf(img->GetWidth());
								++i;
							}
						} else {
							color = GetColor(msg[i]);
							color.w = fade;
						}
					} else {
						size_t ln = 0;
						GetCodePointFromUTF8String(msg, i, &ln);
						if (ln == 0)
							continue;
						ch.resize(ln);
						for (size_t k = 0; k < ln && (i + k) < msg.size(); k++)
							ch[k] = msg[i + k];
						i += ln - 1;

						Vector4 shadow = shadowColor;
						Vector2 pos = MakeVector2(tx + curPosX, ty + winY).Floor();

						if (killfeed) {
							font->DrawShadow(ch, pos + MakeVector2(1, 1), 1.0F, shadow, shadow);
							font->Draw(ch, pos, 1.0F, color);
						} else {
							font->DrawShadow(ch, pos, 1.0F, color, shadow);
						}

						tx += (int)roundf(font->Measure(ch).x);
					}
				}

			endDrawLine:
				y = killfeed ? y + ent.height : entryY;
			}
		}

		bool ChatWindow::HasVisibleMessages() const {
			for (const auto& ent : entries) {
				if (ent.timeFade > 0.0F)
					return true;
			}
			return false;
		}
	} // namespace client
} // namespace spades
