#include "Player.h"
#include "Engine.h"
#include "Textures.h"
#include "Audio.h"
#include "Input.h"
#include "Render.h"
#include "Scene.h"
#include "Log.h"
#include "Physics.h"
#include "EntityManager.h"

Player::Player() : Entity(EntityType::PLAYER)
{
	name = "Player";
}

Player::~Player() {
	pbody->body->GetWorld()->DestroyBody(pbody->body);
}

bool Player::Awake() {

	return true;
}

bool Player::Start() {

	//L03: TODO 2: Initialize Player parameters
	texture = Engine::GetInstance().textures.get()->Load(parameters.attribute("texture").as_string());
	position.setX(parameters.attribute("x").as_int());
	position.setY(parameters.attribute("y").as_int());
	texW = parameters.attribute("w").as_int();
	texH = parameters.attribute("h").as_int();

	//Load animations
	idle.LoadAnimations(parameters.child("animations").child("idle"));
	currentAnimation = &idle;

	// L08 TODO 5: Add physics to the player - initialize physics body
	pbody = Engine::GetInstance().physics.get()->CreateCircle((int)position.getX(), (int)position.getY(), texW / 2, bodyType::DYNAMIC);

	// L08 TODO 6: Assign player class (using "this") to the listener of the pbody. This makes the Physics module to call the OnCollision method
	pbody->listener = this;

	// L08 TODO 7: Assign collider type
	pbody->ctype = ColliderType::PLAYER;

	// Set the gravity of the body
	if (!parameters.attribute("gravity").as_bool()) pbody->body->SetGravityScale(0);

	//initialize audio effect
	pickCoinFxId = Engine::GetInstance().audio.get()->LoadFx("Assets/Audio/Fx/retro-video-game-coin-pickup-38299.ogg");

	return true;
}

bool Player::Update(float dt)
{
	// L08 TODO 5: Add physics to the player - updated player position using physics
	b2Vec2 velocity = b2Vec2(0, pbody->body->GetLinearVelocity().y);

	if (!parameters.attribute("gravity").as_bool()) {
		velocity = b2Vec2(0,0);
	}

	// Move left
	if (Engine::GetInstance().input.get()->GetKey(SDL_SCANCODE_A) == KEY_REPEAT) {
		velocity.x = -0.2 * 8;
	}

	// Move right
	if (Engine::GetInstance().input.get()->GetKey(SDL_SCANCODE_D) == KEY_REPEAT) {
		velocity.x = 0.2 * 8;
	}

	// Move Up
	//if (Engine::GetInstance().input.get()->GetKey(SDL_SCANCODE_W) == KEY_REPEAT) {
	//	velocity.y = -0.2 * 8;
	//}

	// Move down
	if (Engine::GetInstance().input.get()->GetKey(SDL_SCANCODE_S) == KEY_REPEAT) {
		velocity.y = 0.2 * 8;
	}
	
	CameraFollow(dt);

	//Jump
	if (Engine::GetInstance().input.get()->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN && isJumping == false) {
		// Apply an initial upward force
		pbody->body->ApplyLinearImpulseToCenter(b2Vec2(0, -jumpForce), true);
		isJumping = true;
	}

	// If the player is jumpling, we don't want to apply gravity, we use the current velocity prduced by the jump
	if(isJumping == true)
	{
		velocity.y = pbody->body->GetLinearVelocity().y;
	}

	// Apply the velocity to the player
	pbody->body->SetLinearVelocity(velocity);

	b2Transform pbodyPos = pbody->body->GetTransform();
	position.setX(METERS_TO_PIXELS(pbodyPos.p.x) - texH / 2);
	position.setY(METERS_TO_PIXELS(pbodyPos.p.y) - texH / 2);

	Engine::GetInstance().render.get()->DrawTexture(texture, (int)position.getX(), (int)position.getY(), &currentAnimation->GetCurrentFrame());
	currentAnimation->Update();

	return true;
}

bool Player::CleanUp()
{
	LOG("Cleanup player");
	Engine::GetInstance().textures.get()->UnLoad(texture);
	return true;
}

// L08 TODO 6: Define OnCollision function for the player. 
void Player::OnCollision(PhysBody* physA, PhysBody* physB) {
	switch (physB->ctype)
	{
	case ColliderType::PLATFORM:
		isJumping = false;
		break;
	case ColliderType::ITEM:
		Engine::GetInstance().audio.get()->PlayFx(pickCoinFxId);
		Engine::GetInstance().physics.get()->DeletePhysBody(physB); // Deletes the body of the item from the physics world
		break;
	case ColliderType::ENEMY:
		if (pbody->body->GetLinearVelocity().y > 0.4)
		{
			std::cout << "Enemy dead";
			if (canCollide)
			{
				canCollide = false;
				Engine::GetInstance().physics.get()->DeletePhysBody(physB);
				collidetimer = 10;
			}
		}
		else
		{
			std::cout << "Player dead";
			dead = true;
			Engine::GetInstance().physics.get()->DeletePhysBody(physA);
		}
		break;
	}
}

void Player::OnCollisionEnd(PhysBody* physA, PhysBody* physB)
{
	switch (physB->ctype)
	{
	case ColliderType::PLATFORM:
		break;
	case ColliderType::ITEM:
		break;
	case ColliderType::UNKNOWN:
		break;
	}
}

void Player::SetPosition(Vector2D pos) {
	pos.setX(pos.getX() + texW / 2);
	pos.setY(pos.getY() + texH / 2);
	b2Vec2 bodyPos = b2Vec2(PIXEL_TO_METERS(pos.getX()), PIXEL_TO_METERS(pos.getY()));
	pbody->body->SetTransform(bodyPos,0);
}

void Player::Death()
{
	if (collidetimer <= 0)
	{
		canCollide = true;
	}
	else
	{
		collidetimer--;
	}
	if (GetPosition().getY() > 16 * 12)
	{
		dead = true;
	}
	if (dead)
	{
		dead = false;
		Engine::GetInstance().scene.get()->LoadState();
	}
}


Vector2D Player::GetPosition() {
	b2Vec2 bodyPos = pbody->body->GetTransform().p;
	Vector2D pos = Vector2D(METERS_TO_PIXELS(bodyPos.x), METERS_TO_PIXELS(bodyPos.y));
	return pos;
}

void Player::CameraFollow(float dt)
{
	Engine::GetInstance().render.get()->camera.x = Lerp(Engine::GetInstance().render.get()->camera.x, -position.getX() * 4 + 200, 0.1f);
}
float Player::Lerp(float a, float b, float t)
{
	return a + t * (b - a);
}