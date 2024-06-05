#include "CraftThreads.hpp"
#include <TlHelp32.h>
namespace Craft
{

	AllOSThreads::AllOSThreads() noexcept(false)
	{
		// Lock all threads
		HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
		if (hSnap == INVALID_HANDLE_VALUE)
			CRAFT_THROW("Failed to create snapshot");
		
		THREADENTRY32 te;
		te.dwSize = sizeof(te);
		if (!Thread32First(hSnap, &te))
		{
			CloseHandle(hSnap);
			CRAFT_THROW("Failed to get first thread");
		}

		do
		{
			if (te.th32OwnerProcessID == GetCurrentProcessId() && te.th32ThreadID != GetCurrentThreadId())
			{
				HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, te.th32ThreadID);
				if (hThread == 0)
					CloseHandle(hSnap);
				_Analysis_assume_(hThread != 0);
				SuspendThread(hThread);
				CloseHandle(hThread);
			}
		} while (Thread32Next(hSnap, &te));

		CloseHandle(hSnap);
	}
	AllOSThreads::~AllOSThreads() noexcept(false)
	{
		// Unlock all threads
		HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
		if (hSnap == INVALID_HANDLE_VALUE)
			CRAFT_THROW("Failed to create snapshot");

		THREADENTRY32 te;
		te.dwSize = sizeof(te);
		if (!Thread32First(hSnap, &te))
		{
			CloseHandle(hSnap);
			CRAFT_THROW("Failed to get first thread");
		}

		do
		{
			if (te.th32OwnerProcessID == GetCurrentProcessId() && te.th32ThreadID != GetCurrentThreadId())
			{
				HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, te.th32ThreadID);
				if (hThread == nullptr)
					CloseHandle(hSnap);
				_Analysis_assume_(hThread != 0);
				ResumeThread(hThread);
				CloseHandle(hThread);
			}
		} while (Thread32Next(hSnap, &te));

		CloseHandle(hSnap);

	}
}
