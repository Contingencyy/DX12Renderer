#pragma once
#include "Graphics/RenderAPI.h"

template<typename Resource_t>
class ResourceSlotmap
{
public:
	static constexpr uint32_t SLOT_OCCUPIED = 0xFFFFFFFF;

public:
	ResourceSlotmap(std::size_t capacity = 1000)
		: m_Capacity(capacity)
	{
		// Allocate initial slots
		m_Slots = new Slot[capacity];

		// Set NextFree on all slots
		for (std::size_t slot = 0; slot < m_Capacity - 1; ++slot)
		{
			m_Slots[slot].NextFree = (uint32_t)slot + 1;
			m_Slots[slot].Generation = 0;
		}
	}

	~ResourceSlotmap()
	{
		// Release slots, delete[] calls destructors
		delete[] m_Slots;
	}

	template<typename... TArgs>
	RenderResourceHandle Insert(TArgs&&... args)
	{
		// Allocate slot
		RenderResourceHandle handle = AllocateSlot();

		// Construct resource inplace of the new slot
		Slot* slot = &m_Slots[handle.Index];
		new (&slot->Resource) Resource_t(std::forward<TArgs>(args)...);

		// Return handle to the slot
		return handle;
	}

	//RenderResourceHandle Insert(Resource_t&& resource)
	//{
	//	// Allocate slot
	//	RenderResourceHandle handle = AllocateSlot();

	//	// Move data into the slot
	//	Slot* slot = &m_Slots[handle.Index];
	//	new (&slot->Resource) Resource_t(std::move(resource));

	//	// Return handle to the slot
	//	return handle;
	//}

	//RenderResourceHandle Insert(const Resource_t resource)
	//{
	//	// Allocate slot
	//	RenderResourceHandle handle = AllocateSlot();

	//	// Move data into the slot
	//	Slot* slot = &m_Slots[handle.Index];
	//	new (&slot->Resource) Resource_t(resource);

	//	// Return handle to the slot
	//	return handle;
	//}

	void Erase(RenderResourceHandle handle)
	{
		// Get slot 0 (sentinel)
		// Check if resource handle is valid
		// Get the slot from the handle index
		// Check if the handle version is equal to the slot generation
		// Increase the slot generation
		// Set the slot's next free to the sentinels next free
		// Set the sentinels next free to the handle index
		// Call destructor on non trivivally destructible
	}

	Resource_t* Find(RenderResourceHandle handle)
	{
		Resource_t* resource = nullptr;

		// Check if resource handle is valid
		if (RENDER_RESOURCE_HANDLE_VALID(handle))
		{
			Slot* slot = &m_Slots[handle.Index];
			if (handle.Version == slot->Generation)
			{
				resource = &slot->Resource;
			}
		}

		// Return resource if the handle gen is equal to slot gen

		return resource;
	}

private:
	RenderResourceHandle AllocateSlot()
	{
		RenderResourceHandle handle = {};
		Slot* sentinel = &m_Slots[0];

		if (sentinel->NextFree)
		{
			uint32_t index = sentinel->NextFree;

			Slot* slot = &m_Slots[index];
			sentinel->NextFree = slot->NextFree;

			slot->NextFree = SLOT_OCCUPIED;

			handle.Index = index;
			handle.Version = slot->Generation;
		}

		return handle;
	}

private:
	struct Slot
	{
		uint32_t NextFree;
		uint32_t Generation;

		Resource_t Resource;
	};

	Slot* m_Slots;
	std::size_t m_Capacity;

};
