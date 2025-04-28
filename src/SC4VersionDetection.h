/*
 * This file is part of SC4Detokenize, a DLL Plugin for SimCity 4
 * that adds a cheat to display text token values.
 *
 * Copyright (C) 2025 Nicholas Hayes
 *
 * SC4Detokenize is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * SC4Detokenize is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with SC4Detokenize.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#include <cstdint>

class SC4VersionDetection
{
public:

	static SC4VersionDetection& GetInstance();

	uint16_t GetGameVersion() const noexcept;

private:

	SC4VersionDetection();

	uint16_t gameVersion;
};

