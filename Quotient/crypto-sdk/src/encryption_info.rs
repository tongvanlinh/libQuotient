use std::sync::Arc;

use matrix_sdk_common::deserialized_responses::VerificationState;

#[derive(Clone)]
pub(crate) struct EncryptionInfo(
    pub(crate) Arc<matrix_sdk_common::deserialized_responses::EncryptionInfo>,
);

impl EncryptionInfo {
    pub(crate) fn is_verified(&self) -> bool {
        matches!(self.0.verification_state, VerificationState::Verified)
    }
}
