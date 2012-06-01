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

/*! \addtogroup PurchaseLib
 *  @{
 */

/**
 *  @defgroup PurchaseLib Purchase Library
 *  @{
 */

/**
 * @file PurchaseManager.h
 * @author Bogdan Iusco
 * @date 3 May 2012
 *
 * @brief The PurchaseManager manages purchase related events and dispatches
 * them to the target products.
 */

#ifndef IAP_PURCHASE_MANAGER_H_
#define IAP_PURCHASE_MANAGER_H_

#include <MAUtil/String.h>
#include <MAUtil/Map.h>
#include <MAUtil/Environment.h>

namespace IAP
{

	// Forward declarations.
	class Purchase;
	class PurchaseManagerListener;

	/**
	 * Apple App Store URL.
	 * Used to verify a receipt.
	 */
	const MAUtil::String sAppStoreURL =
		"https://buy.itunes.apple.com/verifyReceipt";

	/**
	 * Apple Sandbox URL.
	 * Used to verify a receipt(testing purpose).
	 */
	const MAUtil::String sAppStoreSandboxURL =
		"https://sandbox.itunes.apple.com/verifyReceipt";

	/**
	 * @brief Class that handles purchase related events.
	 */
	class PurchaseManager : public MAUtil::CustomEventListener
	{
	public:
		/**
		*  Check if in app purchase is supported/enabled on a device.
		* @return One of the next result codes:
		* - MA_PURCHASE_RES_OK if purchase is supported/allowed on the device.
		* - MA_PURCHASE_RES_DISABLED if purchase is not allowed/enabled.
		*/
		static int checkPurchaseSupported();

		/**
		 * Return the single instance of this class.
		 */
		static PurchaseManager* getInstance();

		/**
		 * Destroy the single instance of this class.
		 * Call this method only when the application will exit.
		 */
		static void destroyInstance();

		/**
		 * Set your Google Play public key to the application. This enables the
		 * application to verify the signature of the transaction information
		 * that is returned from Google Play.
		 * This method is mandatory for being able to request for purchases.
		 * Platform: Android.
		 * @param developerPublicKey Base64-encoded public key, that can be found
		 * on the Google Play publisher account page, under Licensing & In-app
		 * Billing panel in Edit Profile.
		 */
		void setPublicKey(const MAUtil::String& developerPublicKey);

		/**
		 * Set the store URL used for verifying the receipt on iOS platform.
		 * Platform: iOS.
		 * @param url Recommended values: sAppStoreURL and sAppStoreSandboxURL.
		 */
		void setStoreURL(const MAUtil::String& url);

		/**
		 * Restore transactions that were previously finished so that you can process
		 * them again. For example, your application would use this to allow a user to
		 * unlock previously purchased content onto a new device.
		 * Listeners will be notified when a purchase is restored, or in the case of an error.
		 */
		void restoreTransactions();

		/**
		 * Implementation of CustomEventListener interface.
		 * This method will get called whenever there is a
		 * widget event generated by the system.
		 * @param event The new received event.
		 */
		virtual void customEvent(const MAEvent& event);

		/**
		 * Add a purchase to the map that holds purchases.
		 * The purchase will receive custom events.
		 * @param purchase The purchase that needs to be registered.
		 * The ownership of the purchase is not passed to this method.
		 */
		void registerPurchase(Purchase* purchase);

		/**
		 * Remove a purchase from the map that holds purchases.
		 * The purchase will not receive custom events.
		 * @param product The purchase that needs to be unregistered.
		 */
		void unregisterPurchase(Purchase* purchase);

		/**
		 * Add an event listener for purchase.
		 * Listener will be notified about restored or refunded products.
		 * @param listener The listener that will receive notifications.
		 */
		void addPurchaseListener(PurchaseManagerListener* listener);

		/**
		 * Remove the event listener for purchase.
		 * @param listener The listener that receives purchase notifications.
		 */
		void removePurchaseListener(PurchaseManagerListener* listener);

	private:
		/**
		 * Constructor is private since this is a singleton.
		 */
		PurchaseManager();

		/**
		 * Destructor.
		 */
		virtual ~PurchaseManager();

		/**
		 * Copy constructor.
		 */
		PurchaseManager(const PurchaseManager& purchaseManager);

		/**
		 * Assignment operator.
		 */
		PurchaseManager& operator=(const PurchaseManager& purchaseManager);

		/**
		 * Create a product that has been restored and notifies the listeners.
		 * @param productHandle Handle to the product that has been restored.
		 */
		void createRestoredProduct(MAHandle productHandle);

		/**
		 * Create a product that has been refunded and notifies the listeners.
		 * @param productHandle Handle to the product that has been refunded.
		 */
		void createRefundedProduct(MAHandle productHandle);

		/**
		 * Handle a MA_PURCHASE_EVENT_RESTORE event.
		 * Notifies listeners about the event.
		 * @param purchaseData Event data.
		 */
		void handleProductRestoreEvent(const MAPurchaseEventData& purchaseData);

		/**
		 * Forwards an event to its purchase object.
		 * @param purchaseData purchaseData Event data.
		 */
		void notifyListener(const MAPurchaseEventData& purchaseData);

	private:
		/**
		 * The single instance of this class.
		 */
		static PurchaseManager* sInstance;

		/**
		 * Dictionary of products identified by product handle.
		 */
		MAUtil::Map<MAHandle, Purchase*> mPurchaseMap;

		/**
		 * Array with listeners.
		 */
		MAUtil::Vector<PurchaseManagerListener*> mListeners;

		/**
		 * Store restored and refunded products.
		 */
		MAUtil::Vector<Purchase*> mSpecialProducts;
	};

} // namespace IAP

#endif /* IAP_PURCHASE_MANAGER_H_ */

/*! @} */
