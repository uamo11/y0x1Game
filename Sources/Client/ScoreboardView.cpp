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

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>

#include "CTFGameMode.h"
#include "Client.h"
#include "Fonts.h"
#include "IFont.h"
#include "IImage.h"
#include "IRenderer.h"
#include "MapView.h"
#include "NetClient.h"
#include "Player.h"
#include "ScoreboardView.h"
#include "TCGameMode.h"
#include "World.h"
#include <Core/Debug.h>
#include <Core/Settings.h>
#include <Core/Strings.h>

SPADES_SETTING(cg_minimapPlayerColor);
SPADES_SETTING(cg_hudPlayerCount);

namespace spades {
	namespace client {

		static const Vector4 white = {1, 1, 1, 1};
		static const Vector4 gray = {0.5, 0.5, 0.5, 1};
		static const Vector4 spectatorIdColor = {210.0F / 255, 210.0F / 255, 210.0F / 255, 1}; // Grey
		static const Vector4 spectatorTextColor = {220.0F / 255, 220.0F / 255, 0, 1}; // Goldish yellow
		static const int spectatorTeamId = 255; // Spectators have a team id of 255

		ScoreboardView::ScoreboardView(Client* c) : client(c), renderer(c->GetRenderer()) {
			SPADES_MARK_FUNCTION();
			world = nullptr;
			tc = nullptr;
			ctf = nullptr;

			// Use GUI font if spectator string has special chars
			auto spectatorString = _TrN("Client", "Spectator{1}", "Spectators{1}", "", "");
			auto hasSpecialChar =
			  std::find_if(spectatorString.begin(), spectatorString.end(), [](char ch) {
				  return !(isalnum(static_cast<unsigned char>(ch)) || ch == '_');
			  }) != spectatorString.end();

			spectatorFont = hasSpecialChar
				? c->fontManager->GetMediumFont()
				: c->fontManager->GetSquareDesignFont();

			intelIcon = renderer.RegisterImage("Gfx/Map/Intel.png");
		}

		ScoreboardView::~ScoreboardView() {}

		int ScoreboardView::GetTeamScore(int team) const {
			if (ctf) {
				return ctf->GetTeam(team).score;
			} else if (tc) {
				int num = 0;
				for (int i = 0; i < tc->GetNumTerritories(); i++) {
					if (tc->GetTerritory(i).ownerTeamId == team)
						num++;
				}
				return num;
			} else {
				return 0;
			}
		}

		int ScoreboardView::GetCaptureLimit() const {
			if (ctf) {
				return ctf->GetCaptureLimit();
			} else if (tc) {
				return tc->GetNumTerritories();
			} else {
				return -1;
			}
		}

		Vector4 ScoreboardView::GetTeamColor(int team) {
			return ConvertColorRGBA(world->GetTeamColor(team));
		}

		void ScoreboardView::Draw() {
			SPADES_MARK_FUNCTION();

			world = client->GetWorld();
			if (!world)
				return; // no world

			// TODO: `ctf` and `tc` are only valid throughout the
			// method call's duration. Move them to a new context type
			stmp::optional<IGameMode&> mode = world->GetMode();
			ctf = (mode->ModeType() == IGameMode::m_CTF)
				? dynamic_cast<CTFGameMode*>(mode.get_pointer())
				: NULL;
			tc = (mode->ModeType() == IGameMode::m_TC)
				? dynamic_cast<TCGameMode*>(mode.get_pointer())
				: NULL;

			Handle<IImage> img;
			IFont& font = client->fontManager->GetSquareDesignFont();
			Vector2 pos, size;
			std::string str;

			float sw = renderer.ScreenWidth();
			float sh = renderer.ScreenHeight();

			Vector2 scrCenter = MakeVector2(sw, sh) * 0.5F;

			float contentsWidth = sw + 8.0F;
			float maxContentsWidth = 800.0F;
			if (contentsWidth > maxContentsWidth)
				contentsWidth = maxContentsWidth;

			float contentsHeight = sh - 156.0F;
			float maxContentsHeight = 360.0F;
			if (contentsHeight > maxContentsHeight)
				contentsHeight = maxContentsHeight;

			float teamBarTop = (sh - contentsHeight) * 0.5F;
			float teamBarHeight = 60.0F;
			float contentsLeft = (sw - contentsWidth) * 0.5F;
			float contentsRight = contentsLeft + contentsWidth;
			float contentsHalf = (contentsRight - contentsLeft) * 0.5F;

			int numPlayers = 0;
			int numSpectators = 0;
			int numAliveTeam1 = 0;
			int numAliveTeam2 = 0;
			for (size_t i = 0; i < world->GetNumPlayerSlots(); i++) {
				auto maybePlayer = world->GetPlayer(static_cast<unsigned int>(i));
				if (!maybePlayer)
					continue;
				Player& player = maybePlayer.value();

				// count total players
				numPlayers++;

				// count spectators
				int teamId = player.GetTeamId();
				if (teamId == spectatorTeamId) {
					numSpectators++; 
					continue;
				}

				// count alive players
				if (!player.IsAlive())
					continue;
				if (teamId == 0)
					numAliveTeam1++;
				else if (teamId == 1)
					numAliveTeam2++;
			}

			bool manyPlayers = numPlayers > 32;
			float playersHeight = (manyPlayers ? 335.0F : 315.0F) - teamBarHeight;
			float playersTop = teamBarTop + teamBarHeight;
			float playersBottom = playersTop + playersHeight;

			bool areSpectatorsPresent = numSpectators > 0;
			float spectatorsHeight = areSpectatorsPresent ? 64.0F : 0.0F;
			float spectatorsBottom = playersBottom + spectatorsHeight;

			// draw shadow
			img = renderer.RegisterImage("Gfx/Scoreboard/TopShadow.tga");
			size.y = 32.0F;
			renderer.SetColorAlphaPremultiplied(MakeVector4(0, 0, 0, 0.2F));
			renderer.DrawImage(img, AABB2(0, teamBarTop - size.y, sw, size.y));
			renderer.DrawImage(img, AABB2(0, spectatorsBottom + size.y, sw, -size.y));

			// draw team bar
			img = renderer.RegisterImage("Gfx/White.tga");
			renderer.SetColorAlphaPremultiplied(AdjustColor(GetTeamColor(0), 0.8F, 0.3F));
			renderer.DrawImage(img, AABB2(0, teamBarTop, scrCenter.x, teamBarHeight));
			renderer.SetColorAlphaPremultiplied(AdjustColor(GetTeamColor(1), 0.8F, 0.3F));
			renderer.DrawImage(img, AABB2(scrCenter.x, teamBarTop, scrCenter.x, teamBarHeight));

			img = renderer.RegisterImage("Gfx/Scoreboard/Grunt.png");
			size.x = 120.0F;
			size.y = 60.0F;
			renderer.DrawImage(img, AABB2(contentsLeft, playersTop - size.y, size.x, size.y));
			renderer.DrawImage(img, AABB2(contentsRight, playersTop - size.y, -size.x, size.y));

			// draw team name
			str = world->GetTeamName(0);
			pos.x = contentsLeft + 120.0F;
			pos.y = teamBarTop + 5.0F;
			font.Draw(str, pos + MakeVector2(1, 2), 1.0F, MakeVector4(0, 0, 0, 0.5));
			font.Draw(str, pos, 1.0F, white);

			str = world->GetTeamName(1);
			pos.x = contentsRight - font.Measure(str).x - 120.0F;
			pos.y = teamBarTop + 5.0F;
			font.Draw(str, pos + MakeVector2(1, 2), 1.0F, MakeVector4(0, 0, 0, 0.5));
			font.Draw(str, pos, 1.0F, white);

			// draw alive player count
			if ((int)cg_hudPlayerCount >= 3) {
				img = renderer.RegisterImage("Gfx/User.png");
				IFont& guiFont = client->fontManager->GetGuiFont();

				float iconSize = 12.0F;
				float counterTop = playersTop - 10.0F;

				// team 1
				str = ToString(numAliveTeam1);
				size = guiFont.Measure(str);
				pos.x = scrCenter.x - 5.0F - iconSize;
				pos.y = counterTop - 2.0F - (size.y - iconSize) * 0.5F;
				renderer.SetColorAlphaPremultiplied(white * 0.5F);
				renderer.DrawImage(img, pos);

				pos.x -= size.x + 2.0F;
				pos.y = counterTop - size.y * 0.5F;
				guiFont.Draw(str, pos, 1.0F, MakeVector4(1, 1, 1, 0.5));

				// team 2
				str = ToString(numAliveTeam2);
				pos.x = scrCenter.x + 5.0F;
				pos.y = counterTop - 2.0F - (size.y - iconSize) * 0.5F;
				renderer.SetColorAlphaPremultiplied(white * 0.5F);
				renderer.DrawImage(img, pos);

				pos.x += iconSize + 2.0F;
				pos.y = counterTop - size.y * 0.5F;
				guiFont.Draw(str, pos, 1.0F, MakeVector4(1, 1, 1, 0.5));
			}

			// draw scores
			int capLimit = GetCaptureLimit();
			if (capLimit != -1) {
				str = Format("{0}-{1}", GetTeamScore(0), capLimit);
				pos.x = scrCenter.x - font.Measure(str).x - 15.0F;
				pos.y = teamBarTop + 5.0F;
				font.Draw(str, pos, 1.0F, MakeVector4(1, 1, 1, 0.5));

				str = Format("{0}-{1}", GetTeamScore(1), capLimit);
				pos.x = scrCenter.x + 15.0F;
				font.Draw(str, pos, 1.0F, MakeVector4(1, 1, 1, 0.5));
			}

			// players background
			img = renderer.RegisterImage("Gfx/Scoreboard/PlayersBg.png");
			renderer.SetColorAlphaPremultiplied(MakeVector4(0, 0, 0, 1));
			renderer.DrawImage(img, AABB2(0, playersTop, sw, playersHeight + spectatorsHeight));

			// draw players
			DrawPlayers(0, contentsLeft, playersTop, contentsHalf, playersHeight);
			DrawPlayers(1, (contentsLeft + contentsHalf) - 4.0F, playersTop, contentsHalf, playersHeight);
			if (areSpectatorsPresent)
				DrawSpectators(scrCenter.x, playersBottom);
		}

		struct ScoreboardEntry {
			int id;
			int score;
			std::string name;
			bool alive;
			bool operator<(const ScoreboardEntry& ent) const { return score > ent.score; }
		};

		extern int palette[32][3];

		void ScoreboardView::DrawPlayers(int team, float left, float top, float width, float height) {
			char buf[16];
			Vector2 size;
			
			int numPlayers = 0;
			std::vector<ScoreboardEntry> entries;
			for (size_t i = 0; i < world->GetNumPlayerSlots(); i++) {
				auto maybePlayer = world->GetPlayer(static_cast<unsigned int>(i));
				if (!maybePlayer)
					continue;
				Player& player = maybePlayer.value();
				if (player.GetTeamId() != team)
					continue;

				ScoreboardEntry ent;
				ent.name = player.GetName();
				ent.score = player.GetScore();
				ent.alive = player.IsAlive();
				ent.id = static_cast<int>(i);
				entries.push_back(ent);
				numPlayers++;
			}

			if (numPlayers == 0)
				return;

			std::sort(entries.begin(), entries.end());

			bool manyPlayers = numPlayers > 32;
			IFont& font = manyPlayers
				? client->fontManager->GetSmallFont()
				: client->fontManager->GetGuiFont();
			float rowHeight = manyPlayers ? 12.0F : 24.0F;

			int maxRows = (int)floorf(height / rowHeight);
			int cols = std::max(1, (numPlayers + maxRows - 1) / maxRows);
			maxRows = (numPlayers + cols - 1) / cols;

			int row = 0, col = 0;
			float colWidth = width / (float)cols;

			const int colorMode = cg_minimapPlayerColor;

			auto specStates = GetSpectatorStates();
			std::map<int, std::vector<std::string>> watchingMap;
			for (const auto& st : specStates) {
				if (!st.isFlying && st.targetId >= 0) {
					watchingMap[st.targetId].push_back(st.name);
				}
			}

			for (const auto& ent : entries) {
				float rowY = top + 6.0F + row * rowHeight;
				float colX = left + col * colWidth;

				// draw player id
				snprintf(buf, sizeof(buf), "#%d", ent.id); // FIXME: 1-base?
				size = font.Measure(buf);
				if (colorMode) {
					int colorIndex = ent.id % 32;
					IntVector3 colorplayer = MakeIntVector3(
						palette[colorIndex][0],
						palette[colorIndex][1],
						palette[colorIndex][2]
					);
					Vector4 colorplayerF = ModifyColor(colorplayer);
					font.Draw(buf, MakeVector2(colX + 35.0F - size.x, rowY), 1.0F, colorplayerF);
				} else {
					font.Draw(buf, MakeVector2(colX + 35.0F - size.x, rowY), 1.0F, white);
				}

				// draw player name
				Vector4 color = ent.alive ? white : gray;
				bool isLocal = (stmp::make_optional(ent.id) == world->GetLocalPlayerIndex());
				if (isLocal) {
					color = GetTeamColor(team);

					// make it brighter for darker colors
					float luminosity = color.x + color.y + color.z;
					if (luminosity <= 0.1F)
						color = MakeVector4(0, 1, 1, 1);
					else if (luminosity <= 0.9F)
						color += (white - color) * 0.5F;
				}
				font.Draw(ent.name, MakeVector2(colX + 40.0F, rowY), 1.0F, color);

				// Draw spectator badge next to player name if someone is watching them
				if (watchingMap.count(ent.id)) {
					const auto& watchers = watchingMap[ent.id];
					Vector2 nameSize = font.Measure(ent.name);
					Vector4 badgeColor = isLocal
					    ? MakeVector4(0.20F, 1.0F, 0.40F, 1.0F)
					    : MakeVector4(1.0F, 0.85F, 0.20F, 0.95F);

					float eyeX = colX + 44.0F + nameSize.x;
					float eyeY = rowY + 3.0F;
					float eyeW = 13.0F;
					float eyeH = 9.0F;

					// Draw clean vector spectator eye icon
					renderer.SetColorAlphaPremultiplied(MakeVector4(0.0F, 0.0F, 0.0F, 0.50F).Premultiplied());
					renderer.DrawFilledRect(eyeX + 1.0F, eyeY + 1.0F, eyeX + eyeW + 1.0F, eyeY + eyeH + 1.0F);

					Vector4 scleraCol = MakeVector4(0.95F, 0.98F, 1.0F, 0.95F).Premultiplied();
					Vector4 irisCol = (badgeColor * 0.9F).Premultiplied();
					Vector4 pupilCol = MakeVector4(0.08F, 0.10F, 0.15F, 0.95F).Premultiplied();

					renderer.SetColorAlphaPremultiplied(scleraCol);
					renderer.DrawFilledRect(eyeX + 1.5F, eyeY, eyeX + eyeW - 1.5F, eyeY + eyeH);
					renderer.DrawFilledRect(eyeX, eyeY + 2.0F, eyeX + eyeW, eyeY + eyeH - 2.0F);

					renderer.SetColorAlphaPremultiplied(irisCol);
					renderer.DrawFilledRect(eyeX + 3.5F, eyeY + 1.5F, eyeX + eyeW - 3.5F, eyeY + eyeH - 1.5F);

					renderer.SetColorAlphaPremultiplied(pupilCol);
					renderer.DrawFilledRect(eyeX + 5.0F, eyeY + 2.5F, eyeX + eyeW - 5.0F, eyeY + eyeH - 2.5F);

					std::string badge = " " + watchers[0];
					if (watchers.size() > 1) {
						badge += " +" + ToString(watchers.size() - 1);
					}
					font.Draw(badge, MakeVector2(eyeX + eyeW + 2.0F, rowY), 1.0F, badgeColor);
				}

				// draw player score
				snprintf(buf, sizeof(buf), "%d", ent.score);
				size = font.Measure(buf);
				font.Draw(buf, MakeVector2(colX + colWidth - 5.0F - size.x, rowY), 1.0F, white);

				// draw intel icon
				if (ctf) {
					const auto& t = ctf->GetTeam(team);
					if (t.hasIntel && ent.id == static_cast<int>(t.carrierId)) {
						Vector2 pos = MakeVector2(colX + colWidth - 25.0F - size.x, rowY + 1.0F);
						float pulse = std::max(0.5F, fabsf(sinf(world->GetTime() * 4.0F)));
						renderer.SetColorAlphaPremultiplied(white * pulse);
						renderer.DrawImage(intelIcon, pos.Floor());
					}
				}

				row++;
				if (row >= maxRows) {
					col++;
					row = 0;
				}
			}
		}

		std::vector<ScoreboardView::SpectatorState> ScoreboardView::GetSpectatorStates() const {
			std::vector<SpectatorState> states;
			if (!world)
				return states;

			auto maybeLocal = world->GetLocalPlayer();
			int localId = maybeLocal ? maybeLocal->GetId() : -1;

			for (size_t i = 0; i < world->GetNumPlayerSlots(); i++) {
				auto maybePlayer = world->GetPlayer(static_cast<unsigned int>(i));
				if (!maybePlayer)
					continue;
				Player& player = maybePlayer.value();
				if (player.GetTeamId() != spectatorTeamId)
					continue;

				SpectatorState st;
				st.id = static_cast<int>(i);
				st.name = player.GetName();
				st.watchingLocal = false;
				st.targetId = -1;
				st.targetName = "";
				st.isFlying = true;

				Vector3 specPos = player.GetPosition();
				float closestDist = 3.5F;

				for (size_t j = 0; j < world->GetNumPlayerSlots(); j++) {
					auto maybeTarget = world->GetPlayer(static_cast<unsigned int>(j));
					if (!maybeTarget)
						continue;
					Player& target = maybeTarget.value();
					if (target.IsSpectator() || !target.IsAlive())
						continue;

					float d = (target.GetPosition() - specPos).GetLength();
					if (d < closestDist) {
						closestDist = d;
						st.targetId = static_cast<int>(j);
						st.targetName = target.GetName();
						st.watchingLocal = (st.targetId == localId);
						st.isFlying = false;
					}
				}

				states.push_back(st);
			}
			return states;
		}

		void ScoreboardView::DrawSpectators(float centerX, float top) const {
			IFont& font = client->fontManager->GetGuiFont();

			auto states = GetSpectatorStates();
			if (states.empty())
				return;

			int numSpectators = static_cast<int>(states.size());
			// The catalog entries use `{1}`, i.e. the *second* format argument, so the
			// count is passed twice: once to pick the plural form, once for the
			// placeholder itself. Stored in a std::string: the old fixed `char[64]`
			// buffer plus strcpy() could overflow with long translations.
			std::string specLabel =
			  _TrN("Client", "Spectator{1}", "Spectators{1}", numSpectators, numSpectators);

			auto sizeSpecString = spectatorFont->Measure(specLabel);
			float totalHeaderW = sizeSpecString.x + 22.0F;
			Vector2 pos = MakeVector2(centerX - totalHeaderW * 0.5F + 22.0F, top + 10.0F);

			// Draw spectator eye icon next to header
			float hEyeX = centerX - totalHeaderW * 0.5F;
			float hEyeY = top + 12.0F;
			float hEyeW = 16.0F;
			float hEyeH = 11.0F;
			renderer.SetColorAlphaPremultiplied(MakeVector4(0.0F, 0.0F, 0.0F, 0.60F).Premultiplied());
			renderer.DrawFilledRect(hEyeX + 1.0F, hEyeY + 1.0F, hEyeX + hEyeW + 1.0F, hEyeY + hEyeH + 1.0F);

			renderer.SetColorAlphaPremultiplied(MakeVector4(0.95F, 0.98F, 1.0F, 0.95F).Premultiplied());
			renderer.DrawFilledRect(hEyeX + 2.0F, hEyeY, hEyeX + hEyeW - 2.0F, hEyeY + hEyeH);
			renderer.DrawFilledRect(hEyeX, hEyeY + 2.5F, hEyeX + hEyeW, hEyeY + hEyeH - 2.5F);

			renderer.SetColorAlphaPremultiplied(MakeVector4(0.20F, 0.75F, 1.0F, 0.95F).Premultiplied());
			renderer.DrawFilledRect(hEyeX + 5.0F, hEyeY + 2.5F, hEyeX + hEyeW - 5.0F, hEyeY + hEyeH - 2.5F);

			renderer.SetColorAlphaPremultiplied(MakeVector4(0.08F, 0.10F, 0.15F, 0.95F).Premultiplied());
			renderer.DrawFilledRect(hEyeX + 6.5F, hEyeY + 4.0F, hEyeX + hEyeW - 6.5F, hEyeY + hEyeH - 4.0F);

			spectatorFont->Draw(specLabel, pos + MakeVector2(1, 2), 1.0F,
			                    MakeVector4(0, 0, 0, 0.5));
			spectatorFont->Draw(specLabel, pos, 1.0F, spectatorTextColor);

			float yOffset = pos.y + sizeSpecString.y + 10.0F;

			struct FormattedSpec {
				std::string idStr;
				std::string nameStr;
				std::string statusStr;
				Vector4 statusCol;
				float totalWidth;
			};
			std::vector<FormattedSpec> formatted;
			formatted.reserve(states.size());

			for (const auto& st : states) {
				FormattedSpec item;
				item.idStr = "#" + ToString(st.id);
				item.nameStr = st.name;
				if (st.watchingLocal) {
					item.statusStr = "[Mirandote]";
					item.statusCol = MakeVector4(0.20F, 1.0F, 0.40F, 1.0F); // Vivid green
				} else if (!st.isFlying && st.targetId >= 0) {
					item.statusStr = "[Viendo a " + st.targetName + "]";
					item.statusCol = MakeVector4(1.0F, 0.85F, 0.20F, 1.0F); // Gold
				} else {
					item.statusStr = "[Volando]";
					item.statusCol = MakeVector4(0.60F, 0.85F, 1.0F, 1.0F); // Soft cyan
				}

				float w = font.Measure(item.idStr).x + 4.0F +
				          font.Measure(item.nameStr).x + 5.0F +
				          font.Measure(item.statusStr).x + 16.0F;
				item.totalWidth = w;
				formatted.push_back(item);
			}

			// Render formatted spectators in clean wrapped lines
			const float maxLineWidth = 800.0F;
			std::vector<std::vector<FormattedSpec>> lines;
			std::vector<FormattedSpec> curLine;
			float curLineWidth = 0.0F;

			for (const auto& item : formatted) {
				if (!curLine.empty() && curLineWidth + item.totalWidth > maxLineWidth) {
					lines.push_back(curLine);
					curLine.clear();
					curLineWidth = 0.0F;
				}
				curLine.push_back(item);
				curLineWidth += item.totalWidth;
			}
			if (!curLine.empty()) {
				lines.push_back(curLine);
			}

			float currentRowY = yOffset;
			for (const auto& line : lines) {
				float lineWidth = 0.0F;
				for (const auto& item : line) lineWidth += item.totalWidth;

				float drawX = centerX - lineWidth * 0.5F;
				for (const auto& item : line) {
					auto sId = font.Measure(item.idStr);
					font.Draw(item.idStr, MakeVector2(drawX, currentRowY), 1.0F, spectatorIdColor);
					drawX += sId.x + 4.0F;

					auto sName = font.Measure(item.nameStr);
					font.Draw(item.nameStr, MakeVector2(drawX, currentRowY), 1.0F, white);
					drawX += sName.x + 5.0F;

					auto sStat = font.Measure(item.statusStr);
					font.Draw(item.statusStr, MakeVector2(drawX, currentRowY), 1.0F, item.statusCol);
					drawX += sStat.x + 16.0F;
				}
				currentRowY += 18.0F;
			}
		}
	} // namespace client
} // namespace spades