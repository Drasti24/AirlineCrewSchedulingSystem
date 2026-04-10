#include "pch.h"
#include "CppUnitTest.h"
#include "ScheduleRepository.h"
#include "ReportService.h"
#include "StateMachine.h"
#include <cstring>
#include <filesystem>
#include <fstream>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace AirlineCrewTests
{
    TEST_CLASS(ScheduleRepositoryTests)
    {
    public:

        TEST_METHOD(GetScheduleByPilotId_ValidPilot_ReturnsTrue)
        {
            ScheduleRepository repo;
            repo.LoadSampleData();

            PilotSchedule result{};
            bool found = repo.GetScheduleByPilotId(101, result);

            Assert::IsTrue(found);
            Assert::AreEqual(101, result.pilotId);
            Assert::AreEqual((size_t)2, result.flights.size());
        }

        TEST_METHOD(GetScheduleByPilotId_InvalidPilot_ReturnsFalse)
        {
            ScheduleRepository repo;
            repo.LoadSampleData();

            PilotSchedule result{};
            bool found = repo.GetScheduleByPilotId(999, result);

            Assert::IsFalse(found);
        }

        TEST_METHOD(GetScheduleByPilotId_SecondPilot_ReturnsCorrectData)
        {
            ScheduleRepository repo;
            repo.LoadSampleData();

            PilotSchedule result{};
            bool found = repo.GetScheduleByPilotId(102, result);

            Assert::IsTrue(found);
            Assert::AreEqual(102, result.pilotId);
            Assert::AreEqual((size_t)2, result.flights.size());
        }

        TEST_METHOD(GetScheduleByPilotId_EmptyRepository_ReturnsFalse)
        {
            ScheduleRepository repo;

            PilotSchedule result{};
            bool found = repo.GetScheduleByPilotId(101, result);

            Assert::IsFalse(found);
        }

        TEST_METHOD(AssignFlight_ValidPilot_AddsFlightSuccessfully)
        {
            ScheduleRepository repo;
            repo.LoadSampleData();

            FlightInfo newFlight{};
            newFlight.flightId = 999;
            strcpy_s(newFlight.origin, sizeof(newFlight.origin), "Toronto");
            strcpy_s(newFlight.destination, sizeof(newFlight.destination), "Los Angeles");
            strcpy_s(newFlight.date, sizeof(newFlight.date), "2026-05-01");

            bool success = repo.AssignFlight(101, newFlight);
            Assert::IsTrue(success);

            PilotSchedule result{};
            bool found = repo.GetScheduleByPilotId(101, result);

            Assert::IsTrue(found);
            Assert::AreEqual((size_t)3, result.flights.size());
            Assert::AreEqual(999, result.flights[2].flightId);
        }

        TEST_METHOD(AssignFlight_InvalidPilot_ReturnsFalse)
        {
            ScheduleRepository repo;
            repo.LoadSampleData();

            FlightInfo newFlight{};
            newFlight.flightId = 888;
            strcpy_s(newFlight.origin, sizeof(newFlight.origin), "Toronto");
            strcpy_s(newFlight.destination, sizeof(newFlight.destination), "Miami");
            strcpy_s(newFlight.date, sizeof(newFlight.date), "2026-05-02");

            bool success = repo.AssignFlight(999, newFlight);

            Assert::IsFalse(success);
        }

        TEST_METHOD(RemoveFlight_ExistingFlight_RemovesSuccessfully)
        {
            ScheduleRepository repo;
            repo.LoadSampleData();

            bool removed = repo.RemoveFlight(101, 101);
            Assert::IsTrue(removed);

            PilotSchedule result{};
            bool found = repo.GetScheduleByPilotId(101, result);

            Assert::IsTrue(found);
            Assert::AreEqual((size_t)1, result.flights.size());
        }

        TEST_METHOD(RemoveFlight_InvalidFlight_ReturnsFalse)
        {
            ScheduleRepository repo;
            repo.LoadSampleData();

            bool removed = repo.RemoveFlight(101, 999);

            Assert::IsFalse(removed);
        }

        TEST_METHOD(UpdateFlight_ExistingFlight_UpdatesSuccessfully)
        {
            ScheduleRepository repo;
            repo.LoadSampleData();

            FlightInfo updatedFlight{};
            updatedFlight.flightId = 205;
            strcpy_s(updatedFlight.origin, sizeof(updatedFlight.origin), "Montreal");
            strcpy_s(updatedFlight.destination, sizeof(updatedFlight.destination), "Paris");
            strcpy_s(updatedFlight.date, sizeof(updatedFlight.date), "2026-06-01");

            bool updated = repo.UpdateFlight(101, updatedFlight);
            Assert::IsTrue(updated);

            PilotSchedule result{};
            bool found = repo.GetScheduleByPilotId(101, result);

            Assert::IsTrue(found);

            bool matched = false;
            for (const auto& flight : result.flights)
            {
                if (flight.flightId == 205)
                {
                    matched = true;
                    Assert::AreEqual(0, strcmp(flight.origin, "Montreal"));
                    Assert::AreEqual(0, strcmp(flight.destination, "Paris"));
                    Assert::AreEqual(0, strcmp(flight.date, "2026-06-01"));
                }
            }

            Assert::IsTrue(matched);
        }

        TEST_METHOD(UpdateFlight_InvalidFlight_ReturnsFalse)
        {
            ScheduleRepository repo;
            repo.LoadSampleData();

            FlightInfo updatedFlight{};
            updatedFlight.flightId = 999;
            strcpy_s(updatedFlight.origin, sizeof(updatedFlight.origin), "Montreal");
            strcpy_s(updatedFlight.destination, sizeof(updatedFlight.destination), "Paris");
            strcpy_s(updatedFlight.date, sizeof(updatedFlight.date), "2026-06-01");

            bool updated = repo.UpdateFlight(101, updatedFlight);

            Assert::IsFalse(updated);
        }
    };

    TEST_CLASS(ReportServiceTests)
    {
    public:

        TEST_METHOD(GenerateReport_ReturnsTrue)
        {
            ScheduleRepository repo;
            repo.LoadSampleData();

            ReportService reportService;
            std::string fileName = "test_monthly_report.txt";

            bool result = reportService.GenerateReport(repo, fileName);

            Assert::IsTrue(result);

            if (std::filesystem::exists(fileName))
            {
                std::filesystem::remove(fileName);
            }
        }

        TEST_METHOD(GenerateReport_CreatesFileSuccessfully)
        {
            ScheduleRepository repo;
            repo.LoadSampleData();

            ReportService reportService;
            std::string fileName = "test_monthly_report.txt";

            bool result = reportService.GenerateReport(repo, fileName);

            Assert::IsTrue(result);
            Assert::IsTrue(std::filesystem::exists(fileName));

            if (std::filesystem::exists(fileName))
            {
                std::filesystem::remove(fileName);
            }
        }

        TEST_METHOD(GenerateReport_FileSizeIsGreaterThanOneMB)
        {
            ScheduleRepository repo;
            repo.LoadSampleData();

            ReportService reportService;
            std::string fileName = "test_monthly_report.txt";

            bool result = reportService.GenerateReport(repo, fileName);

            Assert::IsTrue(result);
            Assert::IsTrue(std::filesystem::exists(fileName));

            auto fileSize = std::filesystem::file_size(fileName);

            Assert::IsTrue(fileSize > 1024 * 1024);

            if (std::filesystem::exists(fileName))
            {
                std::filesystem::remove(fileName);
            }
        }
    };

    TEST_CLASS(StateMachineTests)
    {
    public:

        TEST_METHOD(Constructor_InitialStateIsDisconnected)
        {
            StateMachine stateMachine;

            Assert::AreEqual((int)STATE_DISCONNECTED, (int)stateMachine.GetCurrentState());
        }

        TEST_METHOD(DisconnectedToConnected_IsValid)
        {
            StateMachine stateMachine;

            bool isValid = stateMachine.IsValidTransition(STATE_CONNECTED);

            Assert::IsTrue(isValid);
        }

        TEST_METHOD(DisconnectedToAuthenticated_IsInvalid)
        {
            StateMachine stateMachine;

            bool isValid = stateMachine.IsValidTransition(STATE_AUTHENTICATED);

            Assert::IsFalse(isValid);
        }

        TEST_METHOD(SetState_ValidTransition_ChangesState)
        {
            StateMachine stateMachine;

            stateMachine.SetState(STATE_CONNECTED);

            Assert::AreEqual((int)STATE_CONNECTED, (int)stateMachine.GetCurrentState());
        }

        TEST_METHOD(SetState_InvalidTransition_SetsErrorState)
        {
            StateMachine stateMachine;

            stateMachine.SetState(STATE_AUTHENTICATED);

            Assert::AreEqual((int)STATE_ERROR, (int)stateMachine.GetCurrentState());
        }

        TEST_METHOD(ConnectedToAuthenticated_IsValid)
        {
            StateMachine stateMachine;
            stateMachine.SetState(STATE_CONNECTED);

            bool isValid = stateMachine.IsValidTransition(STATE_AUTHENTICATED);

            Assert::IsTrue(isValid);
        }

        TEST_METHOD(ConnectedToDisconnected_IsValid)
        {
            StateMachine stateMachine;
            stateMachine.SetState(STATE_CONNECTED);

            bool isValid = stateMachine.IsValidTransition(STATE_DISCONNECTED);

            Assert::IsTrue(isValid);
        }

        TEST_METHOD(ConnectedToSendingSchedule_IsInvalid)
        {
            StateMachine stateMachine;
            stateMachine.SetState(STATE_CONNECTED);

            bool isValid = stateMachine.IsValidTransition(STATE_SENDING_SCHEDULE);

            Assert::IsFalse(isValid);
        }

        TEST_METHOD(AuthenticatedToProcessingRequest_IsValid)
        {
            StateMachine stateMachine;
            stateMachine.SetState(STATE_CONNECTED);
            stateMachine.SetState(STATE_AUTHENTICATED);

            bool isValid = stateMachine.IsValidTransition(STATE_PROCESSING_REQUEST);

            Assert::IsTrue(isValid);
        }

        TEST_METHOD(AuthenticatedToDisconnected_IsValid)
        {
            StateMachine stateMachine;
            stateMachine.SetState(STATE_CONNECTED);
            stateMachine.SetState(STATE_AUTHENTICATED);

            bool isValid = stateMachine.IsValidTransition(STATE_DISCONNECTED);

            Assert::IsTrue(isValid);
        }

        TEST_METHOD(AuthenticatedToSendingReport_IsInvalid)
        {
            StateMachine stateMachine;
            stateMachine.SetState(STATE_CONNECTED);
            stateMachine.SetState(STATE_AUTHENTICATED);

            bool isValid = stateMachine.IsValidTransition(STATE_SENDING_REPORT);

            Assert::IsFalse(isValid);
        }

        TEST_METHOD(ProcessingRequestToSendingSchedule_IsValid)
        {
            StateMachine stateMachine;
            stateMachine.SetState(STATE_CONNECTED);
            stateMachine.SetState(STATE_AUTHENTICATED);
            stateMachine.SetState(STATE_PROCESSING_REQUEST);

            bool isValid = stateMachine.IsValidTransition(STATE_SENDING_SCHEDULE);

            Assert::IsTrue(isValid);
        }

        TEST_METHOD(ProcessingRequestToSendingReport_IsValid)
        {
            StateMachine stateMachine;
            stateMachine.SetState(STATE_CONNECTED);
            stateMachine.SetState(STATE_AUTHENTICATED);
            stateMachine.SetState(STATE_PROCESSING_REQUEST);

            bool isValid = stateMachine.IsValidTransition(STATE_SENDING_REPORT);

            Assert::IsTrue(isValid);
        }

        TEST_METHOD(ProcessingRequestToAuthenticated_IsValid)
        {
            StateMachine stateMachine;
            stateMachine.SetState(STATE_CONNECTED);
            stateMachine.SetState(STATE_AUTHENTICATED);
            stateMachine.SetState(STATE_PROCESSING_REQUEST);

            bool isValid = stateMachine.IsValidTransition(STATE_AUTHENTICATED);

            Assert::IsTrue(isValid);
        }

        TEST_METHOD(ProcessingRequestToError_IsValid)
        {
            StateMachine stateMachine;
            stateMachine.SetState(STATE_CONNECTED);
            stateMachine.SetState(STATE_AUTHENTICATED);
            stateMachine.SetState(STATE_PROCESSING_REQUEST);

            bool isValid = stateMachine.IsValidTransition(STATE_ERROR);

            Assert::IsTrue(isValid);
        }

        TEST_METHOD(SendingScheduleToAuthenticated_IsValid)
        {
            StateMachine stateMachine;
            stateMachine.SetState(STATE_CONNECTED);
            stateMachine.SetState(STATE_AUTHENTICATED);
            stateMachine.SetState(STATE_PROCESSING_REQUEST);
            stateMachine.SetState(STATE_SENDING_SCHEDULE);

            bool isValid = stateMachine.IsValidTransition(STATE_AUTHENTICATED);

            Assert::IsTrue(isValid);
        }

        TEST_METHOD(SendingReportToAuthenticated_IsValid)
        {
            StateMachine stateMachine;
            stateMachine.SetState(STATE_CONNECTED);
            stateMachine.SetState(STATE_AUTHENTICATED);
            stateMachine.SetState(STATE_PROCESSING_REQUEST);
            stateMachine.SetState(STATE_SENDING_REPORT);

            bool isValid = stateMachine.IsValidTransition(STATE_AUTHENTICATED);

            Assert::IsTrue(isValid);
        }

        TEST_METHOD(ErrorToAuthenticated_IsValid)
        {
            StateMachine stateMachine;

            stateMachine.SetState(STATE_AUTHENTICATED);

            bool isValid = stateMachine.IsValidTransition(STATE_AUTHENTICATED);

            Assert::IsTrue(isValid);
        }

        TEST_METHOD(ErrorToDisconnected_IsValid)
        {
            StateMachine stateMachine;

            stateMachine.SetState(STATE_AUTHENTICATED);

            bool isValid = stateMachine.IsValidTransition(STATE_DISCONNECTED);

            Assert::IsTrue(isValid);
        }

        TEST_METHOD(SendingScheduleToDisconnected_IsInvalid)
        {
            StateMachine stateMachine;
            stateMachine.SetState(STATE_CONNECTED);
            stateMachine.SetState(STATE_AUTHENTICATED);
            stateMachine.SetState(STATE_PROCESSING_REQUEST);
            stateMachine.SetState(STATE_SENDING_SCHEDULE);

            bool isValid = stateMachine.IsValidTransition(STATE_DISCONNECTED);

            Assert::IsFalse(isValid);
        }
    };
}