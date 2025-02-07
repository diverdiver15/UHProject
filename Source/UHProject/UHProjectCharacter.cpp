#include "UHProjectCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include "UHPlayerState.h"
#include "ObjectInterface.h"
#include "MenuHUD.h"
#include "Item.h"
#include "PickUpItem.h"
#include "ItemSlot.h"
#include "UHProjectPlayerController.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);


AUHProjectCharacter::AUHProjectCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
		
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	mItemSlot = CreateDefaultSubobject<UItemSlot>(TEXT("ItemSlot"));
	mItemSlot->SetupAttachment(GetCapsuleComponent());

	mLength = 150.0f;
}

void AUHProjectCharacter::BeginPlay()
{
	Super::BeginPlay();

	mPlayerState = Cast<AUHPlayerState>(GetPlayerState());
	mPlayerController = Cast<AUHProjectPlayerController>(mPlayerState->GetPlayerController());
	mHUD = Cast<AMenuHUD>(mPlayerController->GetHUD());
}

void AUHProjectCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AUHProjectCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AUHProjectCharacter::Look);
		EnhancedInputComponent->BindAction(InteractionAction, ETriggerEvent::Started, this, &AUHProjectCharacter::Interaction);
	}
	else
	{

	}
}

void AUHProjectCharacter::LineTrace()
{
	FHitResult Result;

	FVector CameraLocation;
	FRotator CameraRotation;

	GetController()->GetPlayerViewPoint(CameraLocation, CameraRotation);

	FVector StartVector = CameraLocation;
	FVector EndVector = StartVector + (CameraRotation.Vector() * mLength);

	FCollisionObjectQueryParams Params;
	FCollisionQueryParams CollisionParams;
	Params.AddObjectTypesToQuery(ECollisionChannel::ECC_WorldDynamic);
	CollisionParams.AddIgnoredActor(this);
	
	bool HitResult = GetWorld()->LineTraceSingleByObjectType(Result, StartVector, EndVector, Params, CollisionParams);

	FColor LineColor = HitResult ? FColor::Green : FColor::Red;
	DrawDebugLine(GetWorld(), StartVector, EndVector, LineColor, false, 1.0f, 0, 2.0f);

	if (!HitResult)
	{
		mHUD->RemoveObjectPopUp();
		return;
	}

	mCurrentHitItem = Cast<AItem>(Result.GetActor());

	if (mCurrentHitItem != nullptr)
	{
		mHUD->SetAndShowObjectPopUp(mCurrentHitItem->Name, true);
	}
	else
	{
		mHUD->SetAndShowObjectPopUp(mCurrentHitItem->Name, false);
	}
}


void AUHProjectCharacter::StartLineTrace()
{
	if (mPlayerState == nullptr || mPlayerState->IsLineTraceOn)
		return;

	mPlayerState->IsLineTraceOn = true;
	GetWorld()->GetTimerManager().SetTimer(LineTraceTimer, this, &AUHProjectCharacter::LineTrace, 0.1f, true);
}

void AUHProjectCharacter::StopLineTrace()
{
	if (mPlayerState == nullptr || !mPlayerState->IsLineTraceOn) 
		return;

	mHUD->RemoveObjectPopUp();

	mPlayerState->IsLineTraceOn = false;
	mCurrentHitItem = nullptr;
	GetWorld()->GetTimerManager().ClearTimer(LineTraceTimer);
}

void AUHProjectCharacter::EquipItem(APickUpItem* SelectedItem)
{
	if (mPlayerState->IsEquip) 
	{
		UnEquipItem();
	}

	mEquipItem = SelectedItem;
	mEquipItem->DisableLineTraceCollisionAndPhyiscs();
	mEquipItem->AttachToComponent(mItemSlot, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	mPlayerState->IsEquip = true;
}

void AUHProjectCharacter::UnEquipItem()
{
	mEquipItem->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	mEquipItem->EnableLineTraceCollisionAndPhyiscs();
	mEquipItem = nullptr;
	mPlayerState->IsEquip = false;
}

void AUHProjectCharacter::Move(const FInputActionValue& Value)
{

	if (mPlayerState == nullptr || !mPlayerState->CanMove)
		return;

	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void AUHProjectCharacter::Look(const FInputActionValue& Value)
{
	if (mPlayerState == nullptr || !mPlayerState->CanLook)
		return;

	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AUHProjectCharacter::Interaction(const FInputActionValue& Value)
{
	if (mCurrentHitItem == nullptr || !mPlayerState->IsLineTraceOn)
	{
		UE_LOG(LogTemp, Error, TEXT("Current Hit Actor is Nullptr"))
		return;
	}

	if (mCurrentHitItem->Implements<UObjectInterface>())
	{
		IObjectInterface* ObjectInterface = Cast<IObjectInterface>(mCurrentHitItem);

		if (ObjectInterface)
		{
			IObjectInterface::Execute_Interaction(mCurrentHitItem);
			mCurrentHitItem = nullptr;
			mHUD->RemoveObjectPopUp();
		}
	}
	else 
	{

	}
}
