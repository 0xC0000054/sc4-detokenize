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

#include "CommandParameterSetUtil.h"
#include "cIGZVariant.h"
#include <cstdio>
#include <cinttypes>

namespace
{
	template <typename T>
	void ToString(cRZBaseString& str, T value)
	{
		if constexpr (std::is_same_v<T, bool>)
		{
			if (value)
			{
				str.Append("true", 4);
			}
			else
			{
				str.Append("false", 5);
			}
		}
		else
		{
			const char* formatStr = nullptr;

			if constexpr (std::is_same_v<T, uint8_t>)
			{
				formatStr = "%" PRIu8;
			}
			else if constexpr (std::is_same_v<T, int8_t>)
			{
				formatStr = "%" PRIi8;
			}
			else if constexpr (std::is_same_v<T, uint16_t>)
			{
				formatStr = "%" PRIu16;
			}
			else if constexpr (std::is_same_v<T, int16_t>)
			{
				formatStr = "%" PRIi16;
			}
			else if constexpr (std::is_same_v<T, uint32_t>)
			{
				formatStr = "%" PRIu32;
			}
			else if constexpr (std::is_same_v<T, int32_t>)
			{
				formatStr = "%" PRIi32;
			}
			else if constexpr (std::is_same_v<T, uint64_t>)
			{
				formatStr = "%" PRIu64;
			}
			else if constexpr (std::is_same_v<T, int64_t>)
			{
				formatStr = "%" PRIi64;
			}
			else if constexpr (
				std::is_same_v<T, float>
				|| std::is_same_v<T, double>)
			{
				formatStr = "%f";
			}
			else
			{
				static_assert(false, "Unsupported parameter type for ToString");
			}

			if (formatStr)
			{
				char buffer[1024]{};

				int length = snprintf(buffer, sizeof(buffer), formatStr, value);

				if (length > 0)
				{
					str.Append(buffer, static_cast<uint32_t>(length));
				}
			}
		}
	}

	template <typename T>
	void ArrayToString(cRZBaseString& str, T* values, uint32_t count)
	{
		for (uint32_t i = 0; i < count; i++)
		{
			ToString(str, values[i]);

			if (i < (count - 1))
			{
				str.Append(", ", 2);
			}
		}
	}

	void VariantToString(const cIGZVariant* pVariant, cRZBaseString& output)
	{
		const cIGZVariant::Type type = static_cast<cIGZVariant::Type>(pVariant->GetType());

		switch (type)
		{
		case cIGZVariant::Type::Bool:
			ToString(output, pVariant->GetValBool());
			break;
		case cIGZVariant::Type::BoolArray:
			ArrayToString(output, pVariant->RefBool(), pVariant->GetCount());
			break;
		case cIGZVariant::Type::Uint8:
			ToString(output, pVariant->GetValUint8());
			break;
		case cIGZVariant::Type::Uint8Array:
			ArrayToString(output, pVariant->RefUint8(), pVariant->GetCount());
			break;
		case cIGZVariant::Type::Sint8:
			ToString(output, pVariant->GetValSint8());
			break;
		case cIGZVariant::Type::Sint8Array:
			ArrayToString(output, pVariant->RefSint8(), pVariant->GetCount());
			break;
		case cIGZVariant::Type::Uint16:
			ToString(output, pVariant->GetValUint16());
			break;
		case cIGZVariant::Type::Uint16Array:
			ArrayToString(output, pVariant->RefUint16(), pVariant->GetCount());
			break;
		case cIGZVariant::Type::Sint16:
			ToString(output, pVariant->GetValSint16());
			break;
		case cIGZVariant::Type::Sint16Array:
			ArrayToString(output, pVariant->RefSint16(), pVariant->GetCount());
			break;
		case cIGZVariant::Type::Uint32:
			ToString(output, pVariant->GetValUint32());
			break;
		case cIGZVariant::Type::Uint32Array:
			ArrayToString(output, pVariant->RefUint32(), pVariant->GetCount());
			break;
		case cIGZVariant::Type::Sint32:
			ToString(output, pVariant->GetValSint32());
			break;
		case cIGZVariant::Type::Sint32Array:
			ArrayToString(output, pVariant->RefSint32(), pVariant->GetCount());
			break;
		case cIGZVariant::Type::Uint64:
			ToString(output, pVariant->GetValUint64());
			break;
		case cIGZVariant::Type::Uint64Array:
			ArrayToString(output, pVariant->RefUint64(), pVariant->GetCount());
			break;
		case cIGZVariant::Type::Sint64:
			ToString(output, pVariant->GetValSint64());
			break;
		case cIGZVariant::Type::Sint64Array:
			ArrayToString(output, pVariant->RefSint64(), pVariant->GetCount());
			break;
		case cIGZVariant::Type::Float32:
			ToString(output, pVariant->GetValFloat32());
			break;
		case cIGZVariant::Type::Float32Array:
			ArrayToString(output, pVariant->RefFloat32(), pVariant->GetCount());
			break;
		case cIGZVariant::Type::Float64:
			ToString(output, pVariant->GetValFloat64());
			break;
		case cIGZVariant::Type::Float64Array:
			ArrayToString(output, pVariant->RefFloat64(), pVariant->GetCount());
			break;
		default:
			pVariant->GetValString(output);
			break;
		}
	}
}

void CommandParameterSetUtil::OutputToString(
	cIGZCommandServer* pCommandServer,
	cIGZCommandParameterSet* pParameterSet,
	cRZBaseString& output)
{
	if (pCommandServer && pParameterSet)
	{
		uint32_t parameterCount = pParameterSet->GetParameterCount();

		if (parameterCount > 0)
		{
			if (parameterCount == 1)
			{
				const uint32_t id = pParameterSet->GetFirstParameterID();
				const cIGZVariant* pVariant = pParameterSet->GetParameter(id);

				if (id == cIGZCommandParameterSet::kStatusParameterID)
				{
					ToString(output, pVariant->GetValSint32() == 0);
				}
				else
				{
					VariantToString(pVariant, output);
				}
			}
			else
			{
				pParameterSet->RemoveParameter(cIGZCommandParameterSet::kStatusParameterID);

				parameterCount = pParameterSet->GetParameterCount();

				if (parameterCount > 1)
				{
					pCommandServer->ConvertCommandOutputToString(pParameterSet, output);
				}
				else
				{
					const uint32_t id = pParameterSet->GetFirstParameterID();
					const cIGZVariant* pVariant = pParameterSet->GetParameter(id);

					VariantToString(pVariant, output);
				}
			}
		}
	}
}
