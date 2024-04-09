//
// Copyright 2024 Signal Messenger, LLC
// SPDX-License-Identifier: AGPL-3.0-only
//

import Foundation

public protocol ContactShareManager {
    func validateAndBuild(
        for contactProto: SSKProtoDataMessageContact,
        tx: DBWriteTransaction
    ) throws -> OwnedAttachmentBuilder<OWSContact>

    func validateAndBuild(
        draft: ContactShareDraft,
        tx: DBWriteTransaction
    ) throws -> OwnedAttachmentBuilder<OWSContact>

    func buildProtoForSending(
        from contactShare: OWSContact,
        parentMessage: TSMessage,
        tx: DBReadTransaction
    ) throws -> SSKProtoDataMessageContact
}

public class ContactShareManagerImpl: ContactShareManager {

    private let attachmentManager: TSResourceManager
    private let attachmentStore: TSResourceStore

    public init(
        attachmentManager: TSResourceManager,
        attachmentStore: TSResourceStore
    ) {
        self.attachmentManager = attachmentManager
        self.attachmentStore = attachmentStore
    }

    public func validateAndBuild(
        for contactProto: SSKProtoDataMessageContact,
        tx: DBWriteTransaction
    ) throws -> OwnedAttachmentBuilder<OWSContact> {
        var givenName: String?
        var familyName: String?
        var namePrefix: String?
        var nameSuffix: String?
        var middleName: String?
        if let nameProto = contactProto.name {
            if nameProto.hasGivenName {
                givenName = nameProto.givenName?.stripped
            }
            if nameProto.hasFamilyName {
                familyName = nameProto.familyName?.stripped
            }
            if nameProto.hasPrefix {
                namePrefix = nameProto.prefix?.stripped
            }
            if nameProto.hasSuffix {
                nameSuffix = nameProto.suffix?.stripped
            }
            if nameProto.hasMiddleName {
                middleName = nameProto.middleName?.stripped
            }
        }

        var organizationName: String?
        if contactProto.hasOrganization {
            organizationName = contactProto.organization?.stripped
        }

        let contactName = OWSContactName(
            givenName: givenName,
            familyName: familyName,
            namePrefix: namePrefix,
            nameSuffix: nameSuffix,
            middleName: middleName,
            organizationName: organizationName
        )

        contactName.ensureDisplayName()

        let contact = OWSContact(name: contactName)

        contact.phoneNumbers = contactProto.number.compactMap { OWSContactPhoneNumber(proto: $0) }
        contact.emails = contactProto.email.compactMap { OWSContactEmail(proto: $0) }
        contact.addresses = contactProto.address.compactMap { OWSContactAddress(proto: $0) }

        if
            let avatar = contactProto.avatar,
            let avatarAttachmentProto = avatar.avatar
        {
            let avatarAttachmentBuilder = try attachmentManager.createAttachmentPointerBuilder(
                from: avatarAttachmentProto,
                tx: tx
            )

            return avatarAttachmentBuilder.wrap { _ in
                switch avatarAttachmentBuilder.info {
                case .legacy(let attachmentUniqueId):
                    contact.setLegacyAvatarAttachmentId(attachmentUniqueId)
                    return contact
                case .v2:
                    // Nothing to change; the reference is foreign.
                    return contact
                }
            }
        } else {
            return .withoutFinalizer(contact)
        }
    }

    public func validateAndBuild(
        draft: ContactShareDraft,
        tx: DBWriteTransaction
    ) throws -> OwnedAttachmentBuilder<OWSContact> {
        func buildContact(legacyAttachmentId: String? = nil) -> OWSContact {
            return OWSContact(
                name: draft.name,
                phoneNumbers: draft.phoneNumbers,
                emails: draft.emails,
                addresses: draft.addresses,
                avatarAttachmentId: legacyAttachmentId
            )
        }

        let avatarDataSource: TSResourceDataSource
        if
            let existingAvatarAttachment = draft.existingAvatarAttachment,
            let stream = existingAvatarAttachment.attachment.asResourceStream()
        {
            avatarDataSource = .forwarding(
                existingAttachment: stream,
                with: existingAvatarAttachment.reference
            )
        } else if let avatarImage = draft.avatarImage {
            guard let imageData = avatarImage.jpegData(compressionQuality: 0.9) else {
                throw OWSAssertionError("Failed to get JPEG")
            }
            let mimeType = OWSMimeTypeImageJpeg
            avatarDataSource = .from(
                data: imageData,
                mimeType: mimeType,
                caption: nil,
                renderingFlag: .default,
                sourceFilename: nil
            )
        } else {
            return .withoutFinalizer(buildContact())
        }

        return try attachmentManager.createAttachmentStreamBuilder(
            from: avatarDataSource,
            tx: tx
        ).wrap { attachmentInfo in
            switch attachmentInfo {
            case .legacy(uniqueId: let uniqueId):
                return buildContact(legacyAttachmentId: uniqueId)
            case .v2:
                return buildContact()
            }
        }

    }

    public func buildProtoForSending(
        from contactShare: OWSContact,
        parentMessage: TSMessage,
        tx: DBReadTransaction
    ) throws -> SSKProtoDataMessageContact {

        let contactBuilder = SSKProtoDataMessageContact.builder()

        let nameBuilder = SSKProtoDataMessageContactName.builder()

        if let givenName = contactShare.name.givenName?.strippedOrNil {
            nameBuilder.setGivenName(givenName)
        }
        if let familyName = contactShare.name.familyName?.strippedOrNil {
            nameBuilder.setFamilyName(familyName)
        }
        if let middleName = contactShare.name.middleName?.strippedOrNil {
            nameBuilder.setMiddleName(middleName)
        }
        if let namePrefix = contactShare.name.namePrefix?.strippedOrNil {
            nameBuilder.setPrefix(namePrefix)
        }
        if let nameSuffix = contactShare.name.nameSuffix?.strippedOrNil {
            nameBuilder.setSuffix(nameSuffix)
        }
        if let organizationName = contactShare.name.organizationName?.strippedOrNil {
            contactBuilder.setOrganization(organizationName)
        }
        nameBuilder.setDisplayName(contactShare.name.displayName)

        contactBuilder.setName(nameBuilder.buildInfallibly())

        contactBuilder.setNumber(contactShare.phoneNumbers.compactMap({ $0.proto() }))
        contactBuilder.setEmail(contactShare.emails.compactMap({ $0.proto() }))
        contactBuilder.setAddress(contactShare.addresses.compactMap({ $0.proto() }))

        if
            let avatarResourceRef = attachmentStore.contactShareAvatarAttachment(
                for: parentMessage,
                tx: tx
            ),
            let avatarResource = attachmentStore.fetch(
                [avatarResourceRef.resourceId],
                tx: tx
            ).first,
            let avatarPointer = avatarResource.asTransitTierPointer(),
            let attachmentProto = attachmentManager.buildProtoForSending(
                from: avatarResourceRef,
                pointer: avatarPointer
            )
        {
            let avatarBuilder = SSKProtoDataMessageContactAvatar.builder()
            avatarBuilder.setAvatar(attachmentProto)
            contactBuilder.setAvatar(avatarBuilder.buildInfallibly())
        }

        let contactProto = contactBuilder.buildInfallibly()

        guard !contactProto.number.isEmpty || !contactProto.email.isEmpty || !contactProto.address.isEmpty else {
            throw OWSAssertionError("contact has neither phone, email or address.")
        }

        return contactProto
    }
}

#if TESTABLE_BUILD

public class MockContactShareManager: ContactShareManager {
    public func validateAndBuild(
        for contactProto: SSKProtoDataMessageContact,
        tx: DBWriteTransaction
    ) throws -> OwnedAttachmentBuilder<OWSContact> {
        return .withoutFinalizer(OWSContact())
    }

    public func validateAndBuild(
        draft: ContactShareDraft,
        tx: DBWriteTransaction
    ) throws -> OwnedAttachmentBuilder<OWSContact> {
        return .withoutFinalizer(OWSContact())
    }

    public func buildProtoForSending(
        from contactShare: OWSContact,
        parentMessage: TSMessage,
        tx: DBReadTransaction
    ) throws -> SSKProtoDataMessageContact {
        return SSKProtoDataMessageContact.builder().buildInfallibly()
    }
}

#endif
