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
 along with OpenSpades.	 If not, see <http://www.gnu.org/licenses/>.

 */

#pragma once

#include <list>
#include <string>

#include <Core/Math.h>

namespace spades {
	namespace client {
		class Client;
		class IFont;
		class IImage;
		class IRenderer;

		static const char MsgColorTeam1 = 1;
		static const char MsgColorTeam2 = 2;
		static const char MsgColorTeam3 = 3;
		static const char MsgColorRed = 4;
		static const char MsgColorGreen = 5;
		static const char MsgColorRestore = 6;
		static const char MsgColorGray = 7;
		static const char MsgColorYellow = 8;
		static const char MsgImage = 9;
		static const char MsgTranslateIcon = 14;
		static const char MsgDeadIcon = 15;
		static const char MsgSpecIcon = 16;
		static const char MsgColorMax = MsgSpecIcon;
		static const char MsgColorFriendlyFire = MsgColorRed;
		static const char MsgColorSysInfo = MsgColorYellow;

		class ChatWindow {
			Client* client;
			IRenderer& renderer;
			IFont* font;

			struct ChatEntry {
				uint64_t id;
				std::string msg;
				float height;
				float fade = 0.0F; // usual fade opacity
				float bufferFade = 0.0F;
				float timeFade; // timeout fade opacity

				ChatEntry(uint64_t Id, const std::string& Msg, float Height, float TimeFade)
					: id(Id), msg(Msg), height(Height), timeFade(TimeFade) {}
			};

			std::list<ChatEntry> entries;
			float firstY;
			bool killfeed;

			float GetWidth();
			float GetHeight();
			float GetLineHeight();
			float CalculateMessageHeight(const std::string& msg);

			std::vector<IImage*> killImages;
			IImage* GetKillImage(char);

		public:
			static const char MsgTranslateIcon = spades::client::MsgTranslateIcon;
			static const char MsgDeadIcon = spades::client::MsgDeadIcon;
			static const char MsgSpecIcon = spades::client::MsgSpecIcon;
			ChatWindow(Client*, IFont* font, bool killfeed);
			~ChatWindow();

			uint64_t AddMessage(const std::string&, float fadeTime = -1.0F);
			void UpdateMessage(uint64_t id, const std::string& newMsg);
			static std::string ColoredMessage(const std::string&, char);
			static std::string TeamColorMessage(const std::string&, int);
			static std::string KillImage(int killType, int weaponType = -1);

			/**
			 * Builds the translated-message marker: `MsgTranslateIcon` followed by the
			 * two-letter code of the language the message was written in. The HUD chat
			 * draws the flag of that language, not of the translation target.
			 */
			static std::string TranslateMarker(const std::string& lang);

			/** Returns the color that a message color control byte selects. */
			Vector4 GetColor(char);

			void Update(float dt);
			void Draw();

			bool HasVisibleMessages() const;
		};
	} // namespace client
} // namespace spades
