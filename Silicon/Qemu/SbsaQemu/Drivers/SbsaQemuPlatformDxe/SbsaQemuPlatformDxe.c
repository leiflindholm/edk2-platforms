/** @file
*  FDT client protocol driver for qemu,mach-virt-ahci DT node
*
*  Copyright (c) 2019, Linaro Ltd. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Library/ArmSmcLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/NonDiscoverableDeviceRegistrationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <IndustryStandard/ArmStdSmc.h>

#include <Protocol/FdtClient.h>

/* those probably should go into IndustryStandard/ArmStdSmc.h */
#define SMC_FASTCALL       0x80000000
#define SMC64_FUNCTION     (SMC_FASTCALL   | 0x40000000)
#define SIP_FUNCTION       (SMC64_FUNCTION | 0x02000000)
#define SIP_FUNCTION_ID(n) (SIP_FUNCTION   | (n))

#define SIP_SVC_VERSION  SIP_FUNCTION_ID(1)
#define SIP_SVC_GET_GICD SIP_FUNCTION_ID(2)
#define SIP_SVC_GET_GICR SIP_FUNCTION_ID(3)

EFI_STATUS
EFIAPI
InitializeSbsaQemuPlatformDxe (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS                     Status;
  UINTN                          Size;
  VOID*                          Base;
  UINTN                          Arg0;
  UINTN                          Arg1;
  UINTN                          Result;

  DEBUG ((DEBUG_INFO, "%a: InitializeSbsaQemuPlatformDxe called\n", __FUNCTION__));

  Base = (VOID*)(UINTN)PcdGet64 (PcdPlatformAhciBase);
  ASSERT (Base != NULL);
  Size = (UINTN)PcdGet32 (PcdPlatformAhciSize);
  ASSERT (Size != 0);

  DEBUG ((DEBUG_INFO, "%a: Got platform AHCI %llx %u\n",
          __FUNCTION__, Base, Size));

  Status = RegisterNonDiscoverableMmioDevice (
                   NonDiscoverableDeviceTypeAhci,
                   NonDiscoverableDeviceDmaTypeCoherent,
                   NULL,
                   NULL,
                   1,
                   Base, Size);

  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "%a: NonDiscoverable: Cannot install AHCI device @%p (Staus == %r)\n",
            __FUNCTION__, Base, Status));
    return Status;
  }

  Result = ArmCallSmc0 (SIP_SVC_VERSION, &Arg0, &Arg1, NULL);
  if (Result == SMC_ARCH_CALL_SUCCESS)
  {
        Result = PcdSet32S (PcdPlatformVersionMajor, Arg0);
        ASSERT_EFI_ERROR (Result);
        Result = PcdSet32S (PcdPlatformVersionMinor, Arg1);
        ASSERT_EFI_ERROR (Result);
  }

  Arg0 = PcdGet32 (PcdPlatformVersionMajor);
  Arg1 = PcdGet32 (PcdPlatformVersionMinor);

  DEBUG ((DEBUG_INFO, "Platform version: %d.%d\n", Arg0, Arg1));

  Result = ArmCallSmc0 (SIP_SVC_GET_GICD, &Arg0, NULL, NULL);
  if (Result == SMC_ARCH_CALL_SUCCESS)
  {
        Result = PcdSet32S (PcdGicDistributorBase, Arg0);
        ASSERT_EFI_ERROR (Result);
  }

  Arg0 = PcdGet32 (PcdGicDistributorBase);

  DEBUG ((DEBUG_INFO, "GICD base: 0x%x\n", Arg0));

  Result = ArmCallSmc0 (SIP_SVC_GET_GICR, &Arg0, NULL, NULL);
  if (Result == SMC_ARCH_CALL_SUCCESS)
  {
        Result = PcdSet32S (PcdGicRedistributorsBase, Arg0);
        ASSERT_EFI_ERROR (Result);
  }

  Arg0 = PcdGet32 (PcdGicRedistributorsBase);

  DEBUG ((DEBUG_INFO, "GICR base: 0x%x\n", Arg0));

  return EFI_SUCCESS;
}
