//
//  Copyright (c) 2021 Open Whisper Systems. All rights reserved.
//

import Foundation

@objc
public class CVWallpaperBlurView: ManualLayoutViewWithLayer {

    private var isPreview = false

    private weak var provider: WallpaperBlurProvider?

    private let imageView = CVImageView()
    private let maskLayer = CAShapeLayer()

    private var state: WallpaperBlurState?

    required init() {
        super.init(name: "CVWallpaperBlurView")

        self.clipsToBounds = true

        imageView.contentMode = .scaleAspectFill
        imageView.layer.mask = maskLayer
        imageView.layer.masksToBounds = true
        addSubview(imageView)

        addLayoutBlock { [weak self] _ in
            self?.applyLayout()
        }
    }

    private func applyLayout() {
        // Prevent the layers from animating changes.
        CATransaction.begin()
        CATransaction.setDisableActions(true)

        imageView.frame = imageViewFrame
        maskLayer.frame = imageView.bounds
        let maskPath = UIBezierPath(roundedRect: maskFrame, cornerRadius: layer.cornerRadius)
        maskLayer.path = maskPath.cgPath

        CATransaction.commit()
    }

    @available(swift, obsoleted: 1.0)
    required init(name: String) {
        owsFail("Do not use this initializer.")
    }

    public func configureForPreview() {
        resetContentAndConfiguration()

        self.isPreview = true

        updateIfNecessary()
    }

    public func configure(provider: WallpaperBlurProvider) {
        resetContentAndConfiguration()

        self.isPreview = false
        // TODO: Observe provider changes.
        self.provider = provider

        updateIfNecessary()
    }

    public func updateIfNecessary() {
        guard !isPreview else {
            self.backgroundColor = Theme.backgroundColor
            imageView.isHidden = true
            return
        }
        guard let provider = provider else {
            owsFailDebug("Missing provider.")
            resetContentAndConfiguration()
            return
        }
        guard let state = provider.wallpaperBlurState else {
            resetContent()
            return
        }
        guard state.id != self.state?.id else {
            ensurePositioning()
            return
        }
        self.state = state
        imageView.image = state.image
        imageView.isHidden = false

        ensurePositioning()
    }

    private var imageViewFrame: CGRect = .zero
    private var maskFrame: CGRect = .zero

    private func ensurePositioning() {
        guard !isPreview else {
            return
        }
        guard let state = self.state else {
            resetContent()
            return
        }
        let referenceView = state.referenceView
        self.imageViewFrame = self.convert(referenceView.bounds, from: referenceView)
        self.maskFrame = referenceView.convert(self.bounds, from: self)

        applyLayout()
    }

    private func resetContent() {
        backgroundColor = nil
        imageView.image = nil
        imageView.isHidden = false
        imageViewFrame = .zero
        maskFrame = .zero
        state = nil
    }

    public func resetContentAndConfiguration() {
        isPreview = false
        provider = nil

        resetContent()
    }
}
