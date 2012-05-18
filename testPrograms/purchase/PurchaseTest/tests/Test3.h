/*
 Copyright (C) 2012 MoSync AB

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License,
 version 2, as published by the Free Software Foundation.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 MA 02110-1301, USA.
 */

/**
 * @file Test3.h
 * @author emma
 *
 * @brief The user tries to buy an unavailable product using the wrapper.
 * ( The public key is set on Android, and checkBillingSupported was called).
 * Expected result:
 * - MA_PURCHASE_RES_UNAVAILABLE on devices with no billing support.
 * - iOS: maPurchaseCreate fails with MA_PURCHASE_STATE_PRODUCT_INVALID.
 * - Android: maPurchaseRequest fails with event type MA_PURCHASE_STATE_FAILED
 * and errorCode = MA_PURCHASE_ERROR_INVALID_PRODUCT.
 *
 */

#ifndef PURCHASE_TEST3_H_
#define PURCHASE_TEST3_H_

#include "ITest.h"
#include "../Controller/IApplicationController.h"
#include "../Util.h"

#include "../wrapper/Purchase.h"
#include "../wrapper/PurchaseListener.h"

#define IOS_UNAVAILABLE_PRODUCT_ID ""
#define ANDROID_UNAVAILABLE_PRODUCT_ID "android.test.item_unavailable"

using namespace IAP;

namespace PurchaseTest
{

	/**
	 * @brief Test a purchase of an unavailable product.
	 * Platform: Android and iOS.
	 */
	class Test3: public ITest,
		public IAP::PurchaseListener
	{
	public:
		/**
		 * Constructor.
		 * @param applicationController Will be notified when test's status
		 * changes.
		 */
		Test3(IApplicationController& applicationController);

		/**
		 * Destructor.
		 */
		virtual ~Test3();

		/**
		 * Start the test.
		 */
		virtual void startTest();

		/**
		 * Get the name of the test.
		 * @return Test's name.
		 */
		virtual MAUtil::String getTestName() const;

		// From PurchaseListener:
		/**
		 * Notifies that the product has been validated by the App Store.
		 * NOTE: On Android there is no validation done at this step, if the
		 * product results to be unavailable, then the application will be later
		 * notified via a requestFailed() callback. The errorCode will equal
		 * #MA_PURCHASE_ERROR_INVALID_PRODUCT.
		 * Platform: iOS and Android.
		 * @param purchase The object that sent the event.
		 */
		virtual void productValid(const Purchase& purchase);

		/**
		 * Notifies that the product is not valid on the App Store.
		 * Platform: iOS.
		 * @param purchase The object that sent the event.
		 */
		virtual void productInvalid(const Purchase& purchase);

		/**
		 * Notifies that the transaction has been received by the App Store/
		 * Google Play.
		 * Platform: Android and iOS.
		 * @param purchase The object that sent the event.
		 */
		virtual void requestInProgress(const Purchase& purchase);

		/**
		 * Notifies that the transaction has been successfully processed.
		 * The user should receive the purchased product.
		 * Platform: Android and iOS.
		 * @param purchase The object that sent the event.
		 */
		virtual void requestCompleted(const Purchase& purchase);

		/**
		 * Notifies that the transaction has failed.
		 * Platform: Android and iOS.
		 * @param purchase The object that sent the event.
		 * @param errorCode The reason why it failed.
		 * Note that even if the request fails because it was canceled
		 * (errorCode = MA_PURCHASE_ERROR_CANCELLED), you will still be
		 * able to get a receipt for your purchase.
		 */
		virtual void requestFailed(const Purchase& purchase,
			const int errorCode);
	private:
		/**
		 * Application controller.
		 */
		IApplicationController& mApplicationController;
		Purchase* mPurchase;
	};

} // namespace PurchaseTest

#endif /* PURCHASE_TEST3_H_ */
