// Copyright (c) 2024 CARBIO Project
// SPDX-License-Identifier: MIT

#ifndef CARBIO_CLI_ROLE_H
#define CARBIO_CLI_ROLE_H

#include <cstdint>
#include <string_view>

namespace carbio {

/**
 * @brief Role-Based Access Control (RBAC) roles
 *
 * Six roles matching the RBAC hierarchy:
 * - R1: Primary Owner
 * - R2: Secondary Owner
 * - R3: Family Member
 * - R4: Restricted Driver
 * - R5: Passenger
 * - R6: Service Technician
 */
enum class Role : uint8_t {
    /**
     * @brief R1 - Primary Owner
     * Full access: engine start, fingerprint enrollment,
     * fingerprint deletion, system settings, audit log viewing
     */
    PrimaryOwner = 1,

    /**
     * @brief R2 - Secondary Owner
     * R1 privileges minus system settings and firmware update
     */
    SecondaryOwner = 2,

    /**
     * @brief R3 - Family Member
     * Engine start, climate control, infotainment, but NOT
     * fingerprint administration
     */
    FamilyMember = 3,

    /**
     * @brief R4 - Restricted Driver (Time-limited user)
     * Valid only during specific time periods (e.g., 08:00-18:00),
     * speed limitation may be active
     */
    RestrictedDriver = 4,

    /**
     * @brief R5 - Passenger (Temporary guest)
     * Infotainment system and climate control only,
     * NOT engine start
     */
    Passenger = 5,

    /**
     * @brief R6 - Service Technician
     * Diagnostic access, ECU communication,
     * NOT engine start in normal mode
     */
    ServiceTech = 6
};

/**
 * @brief RBAC permissions in bitflag representation
 *
 * Simplified version of the 30 permissions defined in ch3.tex
 * (10 critical permissions).
 */
enum class Permission : uint32_t {
    // Basic vehicle functions
    VehicleUnlock      = 0x0001,  ///< Unlock vehicle
    EngineStart        = 0x0002,  ///< Start engine
    ClimateControl     = 0x0004,  ///< Climate control settings
    Infotainment       = 0x0008,  ///< Infotainment system access

    // Administrative permissions
    AdminAccess        = 0x0010,  ///< Admin panel access
    EnrollFingerprint  = 0x0020,  ///< Enroll new fingerprint
    DeleteFingerprint  = 0x0040,  ///< Delete fingerprint

    // Audit and diagnostics
    ViewAuditLog       = 0x0080,  ///< View audit log
    ExportAuditLog     = 0x0100,  ///< Export audit log
    DiagnosticAccess   = 0x0200   ///< Diagnostic access
};

/**
 * @brief Role-permission matrix
 *
 * Defines what permissions each role has.
 * Follows the NIST RBAC model (ch2.tex section 3.1).
 *
 * @param role The role
 * @return Bitflags with permissions
 */
constexpr uint32_t role_permissions(Role role) {
    using P = Permission;

    switch (role) {
        case Role::PrimaryOwner:
            // R1: Full access (all bits set)
            return static_cast<uint32_t>(P::VehicleUnlock)
                 | static_cast<uint32_t>(P::EngineStart)
                 | static_cast<uint32_t>(P::ClimateControl)
                 | static_cast<uint32_t>(P::Infotainment)
                 | static_cast<uint32_t>(P::AdminAccess)
                 | static_cast<uint32_t>(P::EnrollFingerprint)
                 | static_cast<uint32_t>(P::DeleteFingerprint)
                 | static_cast<uint32_t>(P::ViewAuditLog)
                 | static_cast<uint32_t>(P::ExportAuditLog)
                 | static_cast<uint32_t>(P::DiagnosticAccess);

        case Role::SecondaryOwner:
            // R2: R1 minus DiagnosticAccess and ExportAuditLog
            return static_cast<uint32_t>(P::VehicleUnlock)
                 | static_cast<uint32_t>(P::EngineStart)
                 | static_cast<uint32_t>(P::ClimateControl)
                 | static_cast<uint32_t>(P::Infotainment)
                 | static_cast<uint32_t>(P::AdminAccess)
                 | static_cast<uint32_t>(P::EnrollFingerprint)
                 | static_cast<uint32_t>(P::DeleteFingerprint)
                 | static_cast<uint32_t>(P::ViewAuditLog);

        case Role::FamilyMember:
            // R3: Driver rights, but NOT admin
            return static_cast<uint32_t>(P::VehicleUnlock)
                 | static_cast<uint32_t>(P::EngineStart)
                 | static_cast<uint32_t>(P::ClimateControl)
                 | static_cast<uint32_t>(P::Infotainment);

        case Role::RestrictedDriver:
            // R4: Driver rights (with time limits, checked elsewhere)
            return static_cast<uint32_t>(P::VehicleUnlock)
                 | static_cast<uint32_t>(P::EngineStart)
                 | static_cast<uint32_t>(P::ClimateControl)
                 | static_cast<uint32_t>(P::Infotainment);

        case Role::Passenger:
            // R5: Passenger functions only, NOT engine start
            return static_cast<uint32_t>(P::ClimateControl)
                 | static_cast<uint32_t>(P::Infotainment);

        case Role::ServiceTech:
            // R6: Diagnostics, but NOT driver rights
            return static_cast<uint32_t>(P::DiagnosticAccess)
                 | static_cast<uint32_t>(P::ViewAuditLog);

        default:
            return 0;  // No permissions
    }
}

/**
 * @brief Check if a role has a specific permission
 *
 * @param role The role
 * @param perm The permission
 * @return true if the role has the permission, false otherwise
 *
 * @example
 * if (has_permission(Role::FamilyMember, Permission::EngineStart)) {
 *     // Engine start allowed
 * }
 */
inline bool has_permission(Role role, Permission perm) {
    return (role_permissions(role) & static_cast<uint32_t>(perm)) != 0;
}

/**
 * @brief Get role name
 *
 * @param role The role
 * @return Role name as text
 */
constexpr std::string_view role_name(Role role) {
    switch (role) {
        case Role::PrimaryOwner:     return "Primary Owner";
        case Role::SecondaryOwner:   return "Secondary Owner";
        case Role::FamilyMember:     return "Family Member";
        case Role::RestrictedDriver: return "Restricted Driver";
        case Role::Passenger:        return "Passenger";
        case Role::ServiceTech:      return "Service Technician";
        default:                     return "Unknown";
    }
}

/**
 * @brief Get permission name
 *
 * @param perm The permission
 * @return Permission name as text
 */
constexpr std::string_view permission_name(Permission perm) {
    switch (perm) {
        case Permission::VehicleUnlock:      return "Vehicle Unlock";
        case Permission::EngineStart:        return "Engine Start";
        case Permission::ClimateControl:     return "Climate Control";
        case Permission::Infotainment:       return "Infotainment";
        case Permission::AdminAccess:        return "Admin Access";
        case Permission::EnrollFingerprint:  return "Enroll Fingerprint";
        case Permission::DeleteFingerprint:  return "Delete Fingerprint";
        case Permission::ViewAuditLog:       return "View Audit Log";
        case Permission::ExportAuditLog:     return "Export Audit Log";
        case Permission::DiagnosticAccess:   return "Diagnostic Access";
        default:                             return "Unknown";
    }
}

}  // namespace carbio

#endif  // CARBIO_CLI_ROLE_H
