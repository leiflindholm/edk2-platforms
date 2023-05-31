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
#include <IndustryStandard/SbsaQemuSmc.h>

#include <Protocol/FdtClient.h>

EFI_STATUS
EFIAPI
InitializeSbsaQemuPlatformDxe (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS                     Status;
  UINTN                          AhciSize, XhciSize;
  VOID                           *AhciBase, *XhciBase;
  UINTN                          Arg0;
  UINTN                          Arg1;
  UINTN                          SmcResult;
  RETURN_STATUS                  Result;

  DEBUG ((DEBUG_INFO, "%a: InitializeSbsaQemuPlatformDxe called\n", __FUNCTION__));

  AhciBase = (VOID*)(UINTN)PcdGet64 (PcdPlatformAhciBase);
  ASSERT (AhciBase != NULL);
  AhciSize = (UINTN)PcdGet32 (PcdPlatformAhciSize);
  ASSERT (AhciSize != 0);

  DEBUG ((DEBUG_INFO, "%a: Got platform AHCI %llx %u\n",
          __FUNCTION__, AhciBase, AhciSize));

  Status = RegisterNonDiscoverableMmioDevice (
                   NonDiscoverableDeviceTypeAhci,
                   NonDiscoverableDeviceDmaTypeCoherent,
                   NULL,
                   NULL,
                   1,
                   AhciBase, AhciSize);

  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "%a: NonDiscoverable: Cannot install AHCI device @%p (Staus == %r)\n",
            __FUNCTION__, AhciBase, Status));
    return Status;
  }

  XhciBase = (VOID*)(UINTN)PcdGet64 (PcdPlatformXhciBase);
  ASSERT (XhciBase != NULL);
  XhciSize = (UINTN)PcdGet32 (PcdPlatformXhciSize);
  ASSERT (XhciSize != 0);

  DEBUG ((DEBUG_INFO, "%a: Got platform XHCI %llx %u\n",
          __FUNCTION__, XhciBase, XhciSize));

  Status = RegisterNonDiscoverableMmioDevice (
                   NonDiscoverableDeviceTypeXhci,
                   NonDiscoverableDeviceDmaTypeCoherent,
                   NULL,
                   NULL,
                   1,
                   XhciBase, XhciSize);

  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "%a: NonDiscoverable: Cannot install XHCI device @%p (Staus == %r)\n",
            __FUNCTION__, XhciBase, Status));
    return Status;
  }

  SmcResult = ArmCallSmc0 (SIP_SVC_VERSION, &Arg0, &Arg1, NULL);
  if (SmcResult == SMC_ARCH_CALL_SUCCESS) {
    Result = PcdSet32S (PcdPlatformVersionMajor, Arg0);
    ASSERT_RETURN_ERROR (Result);
    Result = PcdSet32S (PcdPlatformVersionMinor, Arg1);
    ASSERT_RETURN_ERROR (Result);
  }

  Arg0 = PcdGet32 (PcdPlatformVersionMajor);
  Arg1 = PcdGet32 (PcdPlatformVersionMinor);

  DEBUG ((DEBUG_INFO, "Platform version: %d.%d\n", Arg0, Arg1));

  SmcResult = ArmCallSmc0 (SIP_SVC_GET_GIC, &Arg0, &Arg1, NULL);
  if (SmcResult == SMC_ARCH_CALL_SUCCESS) {
    Result = PcdSet64S (PcdGicDistributorBase, Arg0);
    ASSERT_RETURN_ERROR (Result);
    Result = PcdSet64S (PcdGicRedistributorsBase, Arg1);
    ASSERT_RETURN_ERROR (Result);
  }

  Arg0 = PcdGet64 (PcdGicDistributorBase);

  DEBUG ((DEBUG_INFO, "GICD base: 0x%x\n", Arg0));

  Arg0 = PcdGet64 (PcdGicRedistributorsBase);

  DEBUG ((DEBUG_INFO, "GICR base: 0x%x\n", Arg0));

  SmcResult = ArmCallSmc0 (SIP_SVC_GET_GIC_ITS, &Arg0, NULL, NULL);
  if (SmcResult == SMC_ARCH_CALL_SUCCESS) {
    Result = PcdSet64S (PcdGicItsBase, Arg0);
    ASSERT_RETURN_ERROR (Result);
  }

  Arg0 = PcdGet64 (PcdGicItsBase);

  DEBUG ((DEBUG_INFO, "GICI base: 0x%x\n", Arg0));

  return EFI_SUCCESS;
}
