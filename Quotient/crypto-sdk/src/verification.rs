use matrix_sdk_crypto::{VerificationRequest, types::requests::OutgoingVerificationRequest};

#[derive(Clone)]
pub(crate) struct CreatedSession(
    pub(crate) VerificationRequest,
    pub(crate) OutgoingVerificationRequest,
);

impl CreatedSession {
    pub(crate) fn verification_id(&self) -> String {
        self.0.flow_id().as_str().to_string()
    }
    pub(crate) fn to_device_event_type(&self) -> String {
        if let OutgoingVerificationRequest::ToDevice(request) = &self.1 {
            request.event_type.to_string()
        } else {
            panic!("This is not a to-device event")
        }
    }

    pub(crate) fn to_device_txn_id(&self) -> String {
        if let OutgoingVerificationRequest::ToDevice(request) = &self.1 {
            request.txn_id.to_string()
        } else {
            panic!("This is not a to-device event")
        }
    }

    pub(crate) fn to_device_messages(&self) -> String {
        if let OutgoingVerificationRequest::ToDevice(request) = &self.1 {
            serde_json::to_string(&request.messages).expect("serde to_string should not fail")
        } else {
            panic!("This is not a to-device event")
        }
    }
}

#[derive(Clone)]
pub(crate) struct Emoji(pub(crate) matrix_sdk_crypto::Emoji);

impl Emoji {
    pub(crate) fn symbol(&self) -> String {
        self.0.symbol.to_string()
    }
    pub(crate) fn description(&self) -> String {
        self.0.description.to_string()
    }
}
