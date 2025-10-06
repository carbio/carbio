#ifndef CARBIO_PRINT_HELPER_H
#define CARBIO_PRINT_HELPER_H

#include "carbio/fingerprint.h"

#include <iostream>
#include <string>

inline const char *statusToString(carbio::StatusCode s)
{
  using carbio::StatusCode;
  switch (s)
  {
    case StatusCode::Success:
      return "Ok";
    case StatusCode::PacketReceiveError:
      return "PacketReceiveError";
    case StatusCode::NoFinger:
      return "NoFinger";
    case StatusCode::ImageCaptureFailed:
      return "ImageCaptureFailed";
    case StatusCode::ImageQualityPoor:
      return "ImageQualityPoor";
    case StatusCode::FeatureExtractionFailed:
      return "FeatureExtractionFailed";
    case StatusCode::NoMatch:
      return "NoMatch";
    case StatusCode::NotFound:
      return "NotFound";
    case StatusCode::EnrollMismatch:
      return "EnrollMismatch";
    case StatusCode::InvalidLocation:
      return "InvalidLocation";
    case StatusCode::DatabaseRangeError:
      return "DatabaseRangeError";
    case StatusCode::UploadFeatureFailed:
      return "UploadFeatureFailed";
    case StatusCode::DeviceOrResourceTooBusy:
      return "DeviceOrResourceTooBusy";
    case StatusCode::UploadFailed:
      return "UploadFailed";
    case StatusCode::DeleteFailed:
      return "DeleteFailed";
    case StatusCode::DatabaseClearFailed:
      return "DatabaseClearFailed";
    case StatusCode::PermissionDenied:
      return "PermissionDenied";
    case StatusCode::InvalidImage:
      return "InvalidImage";
    case StatusCode::FlashError:
      return "FlashError";
    case StatusCode::InvalidRegister:
      return "InvalidRegister";
    case StatusCode::Timeout:
      return "Timeout";
    case StatusCode::BadPacket:
      return "BadPacket";
    default:
      return "UnknownStatus";
  }
}

inline void printSystemParams(const carbio::SystemParameters &p)
{
  std::cout << "SystemParameters:\n"
            << "  statusRegister: " << p.statusRegister << "\n"
            << "  systemId:       " << p.systemId << "\n"
            << "  capacity:       " << p.capacity << "\n"
            << "  SecurityLevel:  " << p.SecurityLevel << "\n"
            << "  deviceAddress:  0x" << std::hex << p.deviceAddress << std::dec << "\n"
            << "  packetLength:   " << p.packetLength << "\n"
            << "  baudRate:       " << p.baudRate << "\n";
}

inline void printSearchResult(const carbio::SearchResult &r)
{
  std::cout << "Match: ID=" << r.fingerId << ", confidence=" << r.confidence << "\n";
}

#endif // CARBIO_PRINT_HELPER_H
